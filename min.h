#ifndef MIN_H
#define MIN_H

#include <unordered_map>
#include <map>
#include <queue>
#include <list>
#include <set>
#include "cassert"
#include "iostream"
#include "global.h"
#include "baseCache.h"
#include "sharedDS.h" 

using namespace std;



// Class providing fixed-size (by number of records)
// LRU-replacement cache of a function with signature
// V f(K)

class PageMinCache : public TestCache<uint64_t,cacheAtom>
{
public:
	
	// Constuctor specifies the cached function and
	// the maximum number of records to be stored.
	PageMinCache(
		cacheAtom(*f)(const uint64_t& , cacheAtom),
		size_t c
	) : _fn(f) , _capacity(c) {
		        assert ( _capacity!=0 );
			accessOrdering.pageBaseBuild();
			
	}
	// Obtain value of the cached function for k
	
	uint32_t access(const uint64_t& k  , cacheAtom& value, uint32_t status) {
		assert(_capacity != 0);
		PRINTV(logfile << "Access key: " << k << endl;);
		// Attempt to find existing record
		const typename key_to_value_type::iterator it	= _key_to_value.find(k);
		
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
	
	
	unsigned long long int get_min_key() {
		return (_key_to_value.begin())->first;
	}
	
	unsigned long long int get_max_key() {
		// 			std::map< K, std::pair<cacheAtom,typename key_tracker_type::iterator> >::iterator it;
		return (_key_to_value.rbegin())->first;
	}
	void remove(const uint64_t& k) {
		PRINTV(logfile << "Removing key " << k << endl;);
		assert(0); // not supported for MIN cache 
	}
private:
	// Key to value and key history iterator
	typedef map< uint64_t, cacheAtom > 	key_to_value_type;
	// access ordering list , used to find next reference lineNo
	AccessOrdering accessOrdering; 
	priority_queue<HeapAtom,deque<HeapAtom>,CompHeapAtom> maxHeap;
	// The function to be cached
	cacheAtom(*_fn)(const uint64_t& , cacheAtom);
	// Maximum number of key-value pairs to be retained
	const size_t _capacity;
	
	// Key-to-value lookup
	key_to_value_type _key_to_value;
	
	// Record a fresh key-value pair in the cache
	int insert( uint64_t k, cacheAtom v) {
		PRINTV(logfile << "insert key " << k  << endl;);
		int status = 0;
		// Method is only called on cache misses
		assert(_key_to_value.find(k) == _key_to_value.end());
		
		// Make space if necessary
		if(_key_to_value.size() == _capacity) {
			PRINTV(logfile << "Cache is Full " << _key_to_value.size() << " sectors" << endl;);
			evict();
			status = EVICT;
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
	void evict() {
		// Assert method is never called when cache is empty
		// Identify the key with max lineNo
		HeapAtom maxHeapAtom = maxHeap.top();
		PRINTV(logfile<<" evicting victim key "<< maxHeapAtom.key <<" with next lineNo "<< maxHeapAtom.lineNo << endl;);
		const typename key_to_value_type::iterator it 	= _key_to_value.find(maxHeapAtom.key);
		assert(it != _key_to_value.end());
		// Erase both elements to completely purge record
		_key_to_value.erase(it);
		maxHeap.pop();
	}
};

class BlockMinCache : public TestCache<uint64_t,cacheAtom>
{
public:
	
	// Constuctor specifies the cached function and
	// the maximum number of records to be stored.
	BlockMinCache(
		cacheAtom(*f)(const uint64_t& , cacheAtom),
		size_t c
	) : _fn(f) , _capacity(c) {
		///ARH: Commented for single level cache implementation
		        assert ( _capacity!=0 );
			accessOrdering.blockBaseBuild();
	}
	// Obtain value of the cached function for k
	
	uint32_t access(const uint64_t& k  , cacheAtom& value, uint32_t status) {
		assert(_capacity != 0);
		PRINTV(logfile << "Access key: " << k << endl;);
		// Attempt to find existing record
		const typename key_to_block_type::iterator it	= _key_to_block.find(value.getSsdblkno());
		
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
				tempBlock.insert(k);
				return (status | PAGEMISS );
			}
			else{
				PRINTV(logfile << "Hit on key: " << k << endl;);
				return (status | PAGEHIT );
			}
			// We do have it. Do nothing in MIN cache
		}
		
	} //end access
	void remove(const uint64_t& k) {
		PRINTV(logfile << "Removing key " << k << endl;);
		assert(0); // not supported for MIN cache 
	}

private:
	// Key to value and key history iterator
	typedef set<uint64_t> SsdBlock_type;
	typedef map< uint64_t, SsdBlock_type > 	key_to_block_type;
	// access ordering list , used to find next reference lineNo
	AccessOrdering accessOrdering; 
	priority_queue<HeapAtom,deque<HeapAtom>,CompHeapAtom> maxHeap;
	// The function to be cached
	cacheAtom(*_fn)(const uint64_t& , cacheAtom);
	// Maximum number of key-value pairs to be retained
	const size_t _capacity;
	
	// Key-to-value lookup
	key_to_block_type _key_to_block;
	
	// Record a fresh key-value pair in the cache
	int insert( uint64_t k, cacheAtom v) {
		PRINTV(logfile << "insert key " << k  << endl;);
		int status = 0;
		// Method is only called on cache misses
		assert(_key_to_block.find(v.getSsdblkno()) == _key_to_block.end());
		
		// Make space if necessary
		if(_key_to_block.size() == _capacity/_gConfiguration.ssd2fsblkRatio) {
			PRINTV(logfile << "Cache is Full " << _key_to_block.size() << " blocks" << endl;);
			evict();
			status = EVICT;
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
		// No need to check return,
		// given previous assert.
		return status;
	}
	// Purge the least-recently-used element in the cache
	void evict() {
		// Assert method is never called when cache is empty
		// Identify the key with max lineNo
		HeapAtom maxHeapAtom = maxHeap.top();
		PRINTV(logfile<<" evicting victim block "<< maxHeapAtom.key <<" with next lineNo "<< maxHeapAtom.lineNo << endl;);
		const typename key_to_block_type::iterator it 	= _key_to_block.find(maxHeapAtom.key);
		assert(it != _key_to_block.end());
		// Erase both elements to completely purge record
		_key_to_block.erase(it);
		maxHeap.pop();
	}
};


#endif //end lru_stl
