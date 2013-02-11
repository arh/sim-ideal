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
	
	uint32_t access(const uint64_t& k  , cacheAtom& value, uint32_t status) ;
	
	
	inline unsigned long long int get_min_key() {
		return (_key_to_value.begin())->first;
	}
	
	inline unsigned long long int get_max_key() {
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
	multiset<HeapAtom,CompHeapAtom> maxHeap;
	// The function to be cached
	cacheAtom(*_fn)(const uint64_t& , cacheAtom);
	// Maximum number of key-value pairs to be retained
	const size_t _capacity;

	// Key-to-value lookup
	key_to_value_type _key_to_value;
	
	// Record a fresh key-value pair in the cache
	int insert( uint64_t k, cacheAtom v);
	// Purge the least-recently-used element in the cache
	void evict();
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
			cacheNum = 0;
	}
	// Obtain value of the cached function for k
	
	uint32_t access(const uint64_t& k  , cacheAtom& value, uint32_t status);
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
	multiset<HeapAtom,CompHeapAtom> maxHeap;
	// The function to be cached
	cacheAtom(*_fn)(const uint64_t& , cacheAtom);
	// Maximum number of key-value pairs to be retained
	const size_t _capacity;
	size_t cacheNum;
	// Key-to-value lookup
	key_to_block_type _key_to_block;
	
	// Record a fresh key-value pair in the cache
	int insert( uint64_t k, cacheAtom v);
	// Purge the least-recently-used element in the cache
	void evict();
};


#endif //end lru_stl
