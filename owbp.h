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
	set < cacheAtom,CompCacheAtom,allocator<cacheAtom> > blockSet;
	uint64_t nextRefIndex;
	uint32_t coldPageCounter;
public:
	OwbpCacheBlock(){
		nextRefIndex = 0; 
		coldPageCounter = 0; 
	}
};



typedef bimap< unordered_set_of<cacheAtom,CompCacheAtom> , //SsdBlock_type
HERE
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
			accessOrdering.blockBaseBuild();
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
	BiMapType BiMap;  
	size_t currSize;  // current number of pages in the cache
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
	uint32_t insert( uint64_t k, cacheAtom v);
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



#endif