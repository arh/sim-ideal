#ifndef OWBP_H
#define OWBP_H
#include <unordered_map>
#include <boost/bimap.hpp>
#include <boost/bimap/unordered_set_of.hpp>
#include <boost/bimap/multiset_of.hpp>
// #include <boost/
#include "glob.h"
#include "sharedDS.h"
#include "baseCache.h"

using namespace std; 
using namespace boost;
using namespace bimaps;

class CompCacheAtom{
public:
	bool operator()(  cacheAtom & a ,  cacheAtom & b ){
		assert (a.getSsdblkno() == b.getSsdblkno() );
		return (a.getFsblkno() < b.getFsblkno() );
	}
};


class CompBlkCacheAtom{
public:
	bool operator()(  cacheAtom & a ,  cacheAtom & b ){
		assert (a.getSsdblkno() == b.getSsdblkno() );
		return (a.getFsblkno() < b.getFsblkno() );
	}
};

class OwbpCacheBlock{
private:
	set < cacheAtom,CompCacheAtom,allocator<cacheAtom> > pageSet;
	uint32_t coldPageCounter;
	deque<nextPageRef> futurePageQ; 
	uint64_t BlkID;
public:

	OwbpCacheBlock(deque<nextPageRef> inFuturePageQ, cacheAtom& firstValue){
		clear();
		pageSet.insert(firstValue);
		coldPageCounter=findColdPageCount();
		futurePageQ = inFuturePageQ;
		BlkID = firstValue.getSsdblkno();
	}

	void clear(){
		coldPageCounter = 0;
		pageSet.clear();
		futurePageQ.clear();
		BlkID = 0;
	}
	uint32_t getMinFutureDist(){
		return futurePageQ.front().distance;
	}
	
	uint32_t findColdPageCount(){
		deque<nextPageRef>::iterator it; 
		uint32_t coldness=0;
		for( it = futurePageQ.begin(); it != futurePageQ.end() ; ++ it){
			if( it->distance < _gConfiguration.futureWindowSize ) // max window size
				++ coldness;
			else
				break;
		}
		return coldness;
	}
	uint32_t getColdness(){
		return coldPageCounter;
	}
	
	uint32_t accessPage(uint64_t pageID , cacheAtom& value);
};



typedef bimap< uint64_t , //SsdBlock_ID
		multiset_of<uint32_t> // future distance
		> BiMapType; 
		
typedef BiMapType::value_type BmAtom;




class OwbpCache : public TestCache<uint64_t,cacheAtom>
{
public:
	// Constuctor specifies the cached function and
	// the maximum number of records to be stored.
	OwbpCache(
		cacheAtom(*f)(const uint64_t& , cacheAtom),
		size_t c
	) : _fn(f) , _capacity(c) {
		///ARH: Commented for single level cache implementation
		    assert ( _capacity!=0 );
			currSize = 0; 
	}
	// Obtain value of the cached function for k
	// access to a page 
	uint32_t access(const uint64_t& k  , cacheAtom& value, uint32_t status);
	void remove(const uint64_t& k) {
		PRINTV(logfile << "Removing key " << k << endl;);
		assert(0); // not supported for MIN cache 
	}

private:
	BiMapType BiMap;  //bimap between blkID and blkColdness
	size_t currSize;  // current number of pages in the cache
	map< uint64_t, OwbpCacheBlock > 	blkID_2_DS;//map blkID to blk DS (all blocks in the cache)
	queue<uint64_t> infinitDistBlkIDQ;
	
	
	// The function to be cached
	cacheAtom(*_fn)(const uint64_t& , cacheAtom);
	// Maximum number of key-value pairs to be retained
	const size_t _capacity;
	
	// Record a fresh key-value pair in the cache
	uint32_t insert( uint64_t k, cacheAtom v);
	// Purge the least-recently-used element in the cache
	void evict(); 
	uint32_t blkHitAccess(const uint64_t& PageNo  , cacheAtom& value, uint32_t status, map< uint64_t, OwbpCacheBlock >::iterator it);
	void insertNewBlk( cacheAtom& value);
};



#endif