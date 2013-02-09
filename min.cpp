#include <unordered_map>
#include <queue>
#include "global.h"
#include "min.h"
#include "configuration.h"
#include "parser.h"

using namespace std;

uint32_t PageMinCache::access(const uint64_t& k  , cacheAtom& value, uint32_t status) {
	assert(_capacity != 0);
	PRINTV(logfile << "Access key: " << k << endl;);
	// Attempt to find existing record
	key_to_value_type::iterator it	= _key_to_value.find(k);
	
	if(it == _key_to_value.end()) {
		// We don’t have it:
		PRINTV(logfile << "Miss on key: " << k << endl;);
		// Evaluate function and create new record
		const cacheAtom v = _fn(k, value);
		
		///ARH: write buffer inserts new elements only on write miss
		if(status & WRITE) {
			status |=  insert(k, v);
			PRINTV(logfile << "Insert done on key: " << k << endl;);
		}
		
		return (status | PAGEMISS);
	} else {
		PRINTV(logfile << "Hit on key: " << k << endl;);
		// We do have it. Do nothing in MIN cache
		return (status | PAGEHIT | BLKHIT);
	}
	
} //end access


// Record a fresh key-value pair in the cache
int PageMinCache::insert( uint64_t k, cacheAtom v) {
	PRINTV(logfile << "insert key " << k  << endl;);
	int status = 0;
	// Method is only called on cache misses
	assert(_key_to_value.find(k) == _key_to_value.end());
	
	// Make space if necessary
	if(_key_to_value.size() == _capacity) {
		PRINTV(logfile << "Cache is Full " << _key_to_value.size() << " sectors" << endl;);
		evict();
		status |= EVICT;
	}
	
	// Record key and lineNo in the maxHeap
	uint32_t tempLineNo = v.getLineNo();
	uint32_t nextAccessLineNo = accessOrdering.nextAccess(k,tempLineNo);
	PRINTV(logfile<<"next access to key "<<k<<" is in lineNo "<<nextAccessLineNo<<endl;);
	assert( tempLineNo <= _gConfiguration.maxLineNo|| nextAccessLineNo <= _gConfiguration.maxLineNo || nextAccessLineNo == INF );
	HeapAtom tempHeapAtom(nextAccessLineNo, k);
	maxHeap.push(tempHeapAtom);
	
	// Create the key-value entry,
	// linked to the usage record.
	_key_to_value.insert(make_pair(k,v));
	// No need to check return,
	// given previous assert.
	return status;
}
// Purge the least-recently-used element in the cache
void PageMinCache::evict() {
	// Assert method is never called when cache is empty
	// Identify the key with max lineNo
	HeapAtom maxHeapAtom = maxHeap.top();
	PRINTV(logfile<<" evicting victim key "<< maxHeapAtom.key <<" with next lineNo "<< maxHeapAtom.lineNo << endl;);
	key_to_value_type::iterator it 	= _key_to_value.find(maxHeapAtom.key);
	assert(it != _key_to_value.end());
	// Erase both elements to completely purge record
	_key_to_value.erase(it);
	maxHeap.pop();
}



uint32_t BlockMinCache::access(const uint64_t& k  , cacheAtom& value, uint32_t status) {
	assert(cacheNum <= _capacity);
	PRINTV(logfile << "Access key: " << k << endl;);
	// Attempt to find existing record
	key_to_block_type::iterator it	= _key_to_block.find(value.getSsdblkno());
	
	if(it == _key_to_block.end()) {
		// We don’t have it:
		PRINTV(logfile << "Miss on block: " << value.getSsdblkno() << endl;);
		// Evaluate function and create new record
		const cacheAtom v = _fn(k, value);
		
		///ARH: write buffer inserts new elements only on write miss
		if(status & WRITE) {
			status |=  insert(k, v);
			PRINTV(logfile << "Insert done on key: " << k << endl;);
		}
		
		return (status | BLKMISS);
	} else {
		PRINTV(logfile << "Hit on Block: " << value.getSsdblkno() << endl;);
		status |= BLKHIT;
		
		SsdBlock_type tempBlock;
		SsdBlock_type::iterator pageit;
		
		tempBlock = it->second;
		pageit = tempBlock.find(k);
		
		if(pageit == tempBlock.end() ){
			PRINTV(logfile << "Miss on key: " << k << endl;);
			if( cacheNum == _capacity) {
				PRINTV(logfile << "Cache is Full " << cacheNum << " pages" << endl;);
				evict();
				status |= EVICT;
			}
			tempBlock.insert(k);
			it->second = tempBlock ; 
			++ cacheNum;
			return (status | PAGEMISS );
		}
		else{
			PRINTV(logfile << "Hit on key: " << k << endl;);
			return (status | PAGEHIT );
		}
		// We do have it. Do nothing in MIN cache
	}
	
} //end access


int BlockMinCache::insert( uint64_t k, cacheAtom v) {
	PRINTV(logfile << "insert key " << k  << endl;);
	int status = 0;
	// Method is only called on block cache misses
	assert(_key_to_block.find(v.getSsdblkno()) == _key_to_block.end());
	
	
	// Make space if necessary
	assert( _capacity );
	if( cacheNum == _capacity) {
		PRINTV(logfile << "Cache is Full " << cacheNum << " pages" << endl;);
		evict();
		status |= EVICT;
	}
	
	// Record ssdblkno and lineNo in the maxHeap
	uint32_t tempLineNo = v.getLineNo();
	uint64_t tempSsdblkno = v.getSsdblkno();
	PRINTV(logfile<<"key "<<k<<" is in ssdblock "<< tempSsdblkno<<endl;);
	uint32_t nextAccessLineNo = accessOrdering.nextAccess(tempSsdblkno,tempLineNo);
	PRINTV(logfile<<"next access to block "<<tempSsdblkno<<" is in lineNo "<<nextAccessLineNo<<endl;);
	assert( tempLineNo <= _gConfiguration.maxLineNo|| nextAccessLineNo <= _gConfiguration.maxLineNo || nextAccessLineNo == INF );
	HeapAtom tempHeapAtom(nextAccessLineNo, tempSsdblkno);
	maxHeap.push(tempHeapAtom);
	
	// Create the key-value entry,
	SsdBlock_type tempBlock;
	tempBlock.clear();
	tempBlock.insert(k);
	// linked to the usage record.
	_key_to_block.insert(make_pair(v.getSsdblkno(),tempBlock));
	++ cacheNum;
	// No need to check return,
	// given previous assert.
	return status;
}
// Purge the least-recently-used element in the cache
void BlockMinCache::evict() {
	// Assert method is never called when cache is empty
	// Identify the key with max lineNo
	assert( maxHeap.size() == _key_to_block.size() ); 
	HeapAtom maxHeapAtom = maxHeap.top();
	PRINTV(logfile<<" evicting victim block "<< maxHeapAtom.key <<" with next lineNo "<< maxHeapAtom.lineNo << endl;);
	key_to_block_type::iterator it 	= _key_to_block.find(maxHeapAtom.key);
	assert(it != _key_to_block.end());
	PRINTV(logfile<<" Make "<< (it->second).size()<<" empty space"<<endl;);
	cacheNum -= (it->second).size();
	// Erase both elements to completely purge record
	_key_to_block.erase(it);
	maxHeap.pop();
}