#ifndef OWBP_H
#define OWBP_H

#include <set>
#include "global.h"
#include "sharedDS.h"
#include "baseCache.h"


using namespace std;


class CompCacheAtom
{
public:
	bool operator()(const cacheAtom a ,  const cacheAtom  b) {
		assert(a.getSsdblkno() == b.getSsdblkno());
		return (a.getFsblkno() < b.getFsblkno());
	}
};


class CompBlkCacheAtom
{
public:
	bool operator()(cacheAtom &a ,  cacheAtom &b) {
		assert(a.getSsdblkno() == b.getSsdblkno());
		return (a.getFsblkno() < b.getFsblkno());
	}
};

class OwbpCacheBlockMetaData
{
public:
	uint32_t coldPageCounter;
	uint32_t distance; // min distinct ditance to the next page reference in future window, INF means there is no ref
	uint64_t BlkID;
	size_t validPageCount;
};



class CompOwbpCacheBlockMetaData
{
public:
	// return coldest block. if there are two block with the same coldness value, return a block with largest destance
	bool operator()(const OwbpCacheBlockMetaData &a , const OwbpCacheBlockMetaData &b) {
		if(a.coldPageCounter != b.coldPageCounter) {
			return a.coldPageCounter < b.coldPageCounter;
		}
// 		else if( a.validPageCount != b.validPageCount )
// 			return a.validPageCount > b.validPageCount;  // there might be a chance to see two block with the same coldness value and distance, for this reason I used multiset
//
		else
			return a.distance < b.distance ;
	}
};

typedef set < cacheAtom, CompCacheAtom, allocator<cacheAtom> > PageSetType;
typedef set < cacheAtom, CompCacheAtom, allocator<cacheAtom> >::iterator PageSetIt;
typedef multiset<OwbpCacheBlockMetaData, CompOwbpCacheBlockMetaData, allocator<OwbpCacheBlockMetaData>> ColdHeap_type;
typedef multiset<OwbpCacheBlockMetaData, CompOwbpCacheBlockMetaData, allocator<OwbpCacheBlockMetaData>>::iterator ColdHeapIt;
typedef multiset<OwbpCacheBlockMetaData, CompOwbpCacheBlockMetaData, allocator<OwbpCacheBlockMetaData>>::reverse_iterator ColdHeapRIt;
typedef set<uint64_t>::iterator victimIt;
typedef set<uint64_t>::reverse_iterator victimRIt;

class OwbpCacheBlock
{
private:
	PageSetType pageSet;
	set < uint64_t > coldPageSet;
public:
	OwbpCacheBlockMetaData meta;
	ColdHeapIt coldHeapIt;

	OwbpCacheBlock(cacheAtom &firstValue) {
		clear();
		meta.BlkID = firstValue.getSsdblkno();
		pageSet.insert(firstValue);
		//update meta.distance and coldPageCounter
		updateMetaDataOnPageInsert(firstValue);	// for the first insertion we do always have PageMiss status
	}
	inline size_t getPageSetSize() const {
		return pageSet.size();
	}
	inline PageSetType getPageSet() const {
		return pageSet; 
	}
	inline void clearPageSet() {
		pageSet.clear();
	}

	inline uint64_t getBlkID() const {
		return meta.BlkID;
	}

	void clear() {
		meta.coldPageCounter = 0;
		pageSet.clear();
		meta.BlkID = 0;
		meta.distance = 0;
		meta.validPageCount = 0;
		coldPageSet.clear();
	}
	inline uint32_t getMinFutureDist() const {
		return meta.distance;
	}


	inline uint32_t getColdness() const {
		return meta.coldPageCounter;
	}

	uint32_t readPage(cacheAtom value);
	uint32_t writePage(cacheAtom value);
	uint32_t findPage(cacheAtom value);
	uint32_t updateMetaDataOnPageInsert(const cacheAtom value);
	// check if firstValue is currpage in the memTrace
};



class OwbpCache : public TestCache<uint64_t, cacheAtom>
{
public:
	// Constuctor specifies the cached function and
	// the maximum number of records to be stored.
	OwbpCache(
		cacheAtom(*f)(const uint64_t & , cacheAtom),
		size_t c,
		unsigned levelMinus
	) : _fn(f) , _capacity(c) , levelMinusMinus(levelMinus) {
		///ARH: Commented for single level cache implementation
		assert(_capacity != 0);
		currSize = 0;
	}
	// Obtain value of the cached function for k
	// access to a page
	uint32_t access(const uint64_t &k  , cacheAtom &value, uint32_t status);
	void remove(const uint64_t &k) {
		PRINTV(logfile << "Removing key " << k << endl;);
		assert(0); // not supported for MIN cache
	}

private:

	size_t currSize;  // current number of pages in the cache
	map< uint64_t, OwbpCacheBlock > 	blkID_2_DS;//map blkID to blk DS (all blocks in the cache)
	ColdHeap_type coldHeap;
	set<uint64_t> victimPull; // maintain block IDs with INF distance
	double currentTime; //arrival time of the latest request, used to generate outTrace



	// The function to be cached
	cacheAtom(*_fn)(const uint64_t & , cacheAtom);
	// Maximum number of key-value pairs to be retained
	const size_t _capacity;
	unsigned levelMinusMinus;

	// Purge the least-recently-used element in the cache
	void evict(uint64_t currBlkID);
	uint32_t blkHitAccess(const uint64_t &k  , cacheAtom &value, uint32_t status, OwbpCacheBlock &tempBlock);
	void insertNewBlk(cacheAtom &value);
	
	//FIXME:this function is here temporary, it record victim entries from evit()
	/* utlimately it should move to the main simulator loop */
	void recordOutTrace( PageSetType victimPageSet );
};



#endif