
#include <queue>
#include "global.h"
#include "min.h"
#include "configuration.h"
#include "parser.h"

using namespace std;

uint32_t PageMinCache::access(const uint64_t &k  , cacheAtom &value, uint32_t status)
{
    assert(_capacity != 0);
    PRINTV(logfile << value.getLineNo() << ": Access key: " << k << endl;);
    // Attempt to find existing record
    key_to_value_type::iterator it	= _key_to_value.find(k);

    if(it == _key_to_value.end()) {
        // We don’t have it:
        PRINTV(logfile << "\tMiss on key: " << k << endl;);
        // Evaluate function and create new record
        const cacheAtom v = _fn(k, value);

        ///ARH: write buffer inserts new elements only on write miss
        if(status & WRITE) {
            status |=  insert(k, v);
            PRINTV(logfile << "\tInsert done on key: " << k << endl;);
        }

        status |= PAGEMISS;
    }
    else {
        PRINTV(logfile << "\tHit on key: " << k << endl;);
        // We do have it. Do nothing in MIN cache
        status |= PAGEHIT | BLKHIT;
        //update maxHeap
        HeapAtom currHeapAtom;
        currHeapAtom.lineNo = value.getLineNo();
        currHeapAtom.key = k;
        uint32_t nextAccessLineNo = 0;
// 		deque<reqAtom>::iterator memit = memTrace.begin();
//
// 		assert(memit->lineNo == currHeapAtom.lineNo );
//
// 		if( (++memit)->lineNo != currHeapAtom.lineNo ){ //if it need update do update
        nextAccessLineNo = accessOrdering.nextAccess(currHeapAtom.key, currHeapAtom.lineNo);
        PRINTV(logfile << "\tNext access to pageID " << k << " is in lineNo " << nextAccessLineNo << endl;);
        // update max heap setit
        bool updated = false;
        multiset<HeapAtom>::iterator setit;
        pair<multiset<HeapAtom>::iterator, multiset<HeapAtom>::iterator> range;
        range = maxHeap.equal_range(currHeapAtom);

        if(range.first != range.second) {
            for(setit = range.first; setit != range.second ; ++setit ) {
                if(setit->key == k ) {
                    maxHeap.erase(setit);
                    currHeapAtom.lineNo = nextAccessLineNo;
                    multiset<HeapAtom>::iterator debugit;
                    debugit = maxHeap.insert(currHeapAtom);
                    assert( debugit != maxHeap.end());
                    IFDEBUG(updated = true;);
                    break;
                }
            }

            assert(updated == true);
        }
        else {
            assert(0);
        }

// 		}
    }

    return status;
} //end access


// Record a fresh key-value pair in the cache
int PageMinCache::insert( uint64_t k, cacheAtom v)
{
    PRINTV(logfile << "\tinsert key " << k  << endl;);
    int status = 0;
    // Method is only called on cache misses
    assert(_key_to_value.find(k) == _key_to_value.end());

    // Make space if necessary
    if(_key_to_value.size() == _capacity) {
        PRINTV(logfile << "\tCache is Full " << _key_to_value.size() << " sectors" << endl;);
        evict();
        status |= EVICT;
    }

    uint32_t tempLineNo = v.getLineNo();
    uint32_t nextAccessLineNo = 0;
// 	deque<reqAtom>::iterator it = memTrace.begin();
//
// 	assert(it->lineNo == tempLineNo);
//
// 	if((++it)->lineNo == tempLineNo ){ // both current request and next one blong to the same block (sequential)
// 		nextAccessLineNo = tempLineNo;
// 	}
// 	else{
    // Record key and lineNo in the maxHeap
    nextAccessLineNo = accessOrdering.nextAccess(k, tempLineNo);
// 	}
    PRINTV(logfile << "\tnext access to pageID " << k << " is in lineNo " << nextAccessLineNo << endl;);
    assert(  nextAccessLineNo <= _gConfiguration.maxLineNo || nextAccessLineNo == INF );
    HeapAtom tempHeapAtom(nextAccessLineNo, k);
    //lookup in heap to find out block hit and update block-level metadate in case of block hit
    multiset<HeapAtom>::iterator setit;
    setit = maxHeap.insert(tempHeapAtom);
    assert( setit != maxHeap.end() );
    // Create the key-value entry,
    // linked to the usage record.
    _key_to_value.insert(make_pair(k, v));
    // No need to check return,
    // given previous assert.
    return status;
}
// Purge the least-recently-used element in the cache
void PageMinCache::evict()
{
    // Assert method is never called when cache is empty
    // Identify the key with max lineNo
    multiset<HeapAtom>::iterator setit = maxHeap.begin();
// 	HeapAtom maxHeapAtom = maxHeap.begin();
    PRINTV(logfile << "\tevicting victim pageID " << setit->key << " with next lineNo " << setit->lineNo << endl;);
    key_to_value_type::iterator it 	= _key_to_value.find(setit->key);
    assert(it != _key_to_value.end());
    // Erase both elements to completely purge record
    _key_to_value.erase(it);
    maxHeap.erase(setit);
}



uint32_t BlockMinCache::access(const uint64_t &k  , cacheAtom &value, uint32_t status)
{
    assert(cacheNum <= _capacity);
    PRINTV(logfile << value.getLineNo() << ": Access key: " << k << endl;);
    // Attempt to find existing record
    key_to_block_type::iterator it	= _key_to_block.find(value.getSsdblkno());

    if(it == _key_to_block.end()) {
        // We don’t have it:
        PRINTV(logfile << "\tMiss on block: " << value.getSsdblkno() << endl;);
        // Evaluate function and create new record
        const cacheAtom v = _fn(k, value);

        ///ARH: write buffer inserts new elements only on write miss
        if(status & WRITE) {
            status |=  insert(k, v);
            PRINTV(logfile << "\tInsert done on key: " << k << endl;);
        }

        status |= BLKMISS;
    }
    else {
        PRINTV(logfile << "\tHit on Block: " << value.getSsdblkno() << endl;);
        status |= BLKHIT;
        assert(it->second.size());
// 		SsdBlock_type tempBlock(it->second);
        SsdBlock_type::iterator pageit;
        pageit = it->second.find(k);

        if(pageit == it->second.end() ) {
            PRINTV(logfile << "\tMiss on key: " << k << endl;);

            if( cacheNum == _capacity) {
                PRINTV(logfile << "\tCache is Full " << cacheNum << " pages" << endl;);
                evict(value.getSsdblkno());
                status |= EVICT;
            }

            (it->second).insert(k);
// 			it->second.swap(tempBlock) ;
            assert(it->second.size() );
            ++ cacheNum;
            status |= PAGEMISS ;
        }
        else {
            // We do have it. Do nothing in MIN cache
            PRINTV(logfile << "\tHit on key: " << k << endl;);
            status |= PAGEHIT ;
        }

        //update maxHeap
        HeapAtom currHeapAtom;
        currHeapAtom.lineNo = value.getLineNo();
        currHeapAtom.key = value.getSsdblkno();
        uint32_t nextAccessLineNo = 0;
        deque<reqAtom>::iterator memit = memTrace.begin();
        assert(memit->lineNo == currHeapAtom.lineNo );

        if( (++memit)->lineNo != currHeapAtom.lineNo ) { //if it need update do update
            nextAccessLineNo = accessOrdering.nextAccess(currHeapAtom.key, currHeapAtom.lineNo);
            PRINTV(logfile << "\tNext access on Block: " << value.getSsdblkno() << " is in line " << nextAccessLineNo << endl;);
            // update max heap setit
            multiset<HeapAtom>::iterator setit;
            setit = maxHeap.find(currHeapAtom);
            assert( setit != maxHeap.end());
            maxHeap.erase(setit);
            currHeapAtom.lineNo = nextAccessLineNo;
            setit = maxHeap.insert(currHeapAtom);
            assert( setit != maxHeap.end());
        }
    } //end hit

    return status;
} //end access


int BlockMinCache::insert( uint64_t k, cacheAtom v)
{
    PRINTV(logfile << "\tinsert key " << k  << endl;);
    int status = 0;
    // Method is only called on block cache misses
    assert(_key_to_block.find(v.getSsdblkno()) == _key_to_block.end());
    // Make space if necessary
    assert( _capacity );

    if( cacheNum == _capacity) {
        PRINTV(logfile << "\tCache is Full " << cacheNum << " pages" << endl;);
        evict(v.getSsdblkno());
        status |= EVICT;
    }

    // Record ssdblkno and lineNo in the maxHeap
    uint64_t tempSsdblkno = v.getSsdblkno();
    PRINTV(logfile << "\tkey " << k << " is in ssdblock " << tempSsdblkno << endl;);
    uint32_t tempLineNo = v.getLineNo();
    uint32_t nextAccessLineNo = 0;
    deque<reqAtom>::iterator it = memTrace.begin();
    assert(it->lineNo == tempLineNo );

    if( (++it)->lineNo == tempLineNo ) {
        nextAccessLineNo = tempLineNo; // sequential requests
    }
    else {
        nextAccessLineNo = accessOrdering.nextAccess(tempSsdblkno, tempLineNo);
    }

    PRINTV(logfile << "\tnext access to block " << tempSsdblkno << " is in lineNo " << nextAccessLineNo << endl;);
    assert( nextAccessLineNo <= _gConfiguration.maxLineNo || nextAccessLineNo == INF );
    HeapAtom tempHeapAtom(nextAccessLineNo, tempSsdblkno);
    multiset<HeapAtom>::iterator setit;
    setit = maxHeap.insert(tempHeapAtom);
    assert(setit != maxHeap.end() );
    // Create the key-value entry,
    SsdBlock_type tempBlock;
    tempBlock.clear();
    tempBlock.insert(k);
    assert(tempBlock.size() == 1);
    // linked to the usage record.
    _key_to_block.insert(make_pair(v.getSsdblkno(), tempBlock));
    ++ cacheNum;
    // No need to check return,
    // given previous assert.
    return status;
}
// Purge the least-recently-used element in the cache
void BlockMinCache::evict(uint64_t ssdblkno)
{
    // Assert method is never called when cache is empty
    // Identify the key with max lineNo
    assert( maxHeap.size() == _key_to_block.size() );
    multiset<HeapAtom>::iterator setit = maxHeap.begin();
    PRINTV(logfile << "\tevicting victim blockID " << setit->key << " with next lineNo " << setit->lineNo << endl;);
    key_to_block_type::iterator it 	= _key_to_block.find(setit->key);
    assert(it != _key_to_block.end());
    assert((it->second).size());
    PRINTV(logfile << "\tMake " << (it->second).size() << " empty space" << endl;);
    cacheNum -= (it->second).size();
    (it->second).clear();

    if( setit->key != ssdblkno ) { //it is possible to evict the same block that currant page blong to it.
        // Erase both elements to completely purge record
        _key_to_block.erase(it);
        maxHeap.erase(setit);
    }
    else {
        PRINTV(logfile << "\tBlock " << setit->key << " cleaned but remains in cache" << endl;);
    }
}