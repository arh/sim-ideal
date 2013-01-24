#include "owbp.h"

void OwbpCache::insertNewBlk(cacheAtom& value){
	
	
	uint64_t currSsdBlkNo = value.getSsdblkno();
	uint32_t currLine = value.getLineNo();
	deque<nextPageRef> nextPages;
	pair< map<uint64_t,OwbpCacheBlock>::iterator, bool> ret;
	nextPages = getFuturePageRef(currSsdBlkNo, currLine);
	
	OwbpCacheBlock tempBlock(nextPages,value);
	
	//insert in bimap
	BmAtom bmAtom(currSsdBlkNo, tempBlock.getColdness() );
	BiMap.insert( bmAtom ); 
	
	//insert in blkID_2_DS 111
	ret = blkID_2_DS.insert(pair<uint64_t,OwbpCacheBlock>(currSsdBlkNo,tempBlock));
	
	
	assert ( ret.second == true ) ; 
	
	
}

// access to a page 
uint32_t OwbpCache::access(const uint64_t& k  , cacheAtom& value, uint32_t status) {
	
	assert(_capacity != 0);
	assert(k == value.getFsblkno());
	PRINTV(logfile << "Access page number: " << k << endl;);
	uint64_t currSsdBlkNo = value.getSsdblkno();
	// Attempt to find existing block 
	map< uint64_t, OwbpCacheBlock >::iterator 	blkit;
	blkit = blkID_2_DS.find(currSsdBlkNo);
	
	if( blkit ==  blkID_2_DS.end() ) {
		// Block miss
		status |= BLKMISS | PAGEMISS;
		if(status & WRITE){
			
			if(currSize < _capacity ){
				evict();
				status |= EVICT;
			}
			insertNewBlk(value);
			++currSize;
		}
	}
	else{
		//Block Hit
		status |= BLKHIT;
		size_t valid_pages;
		if(status & WRITE){
			
			//PAGEMISS or hit to status is return value of this function
			IFDEBUG( valid_pages = blkit->second.getPageSetSize(); );
			status |= blkit->second.writePage(value);
			IFDEBUG( assert( valid_pages + 1 == blkit->second.getPageSetSize() ););
			
			///TODO: update metadata and bimap
			if( status & PAGEMISS)
				++ currSize;
		}
		else{ // read a page 
			IFDEBUG( valid_pages = blkit->second.getPageSetSize(); );
			status |= blkit->second.readPage(value);
			IFDEBUG( assert( valid_pages == blkit->second.getPageSetSize() ););
			
		}
	}
	
	return status;	
} //end access


void OwbpCache::evict(){
// 	PRINTV(logfile<<" evicting victim block "<< maxHeapAtom.key <<" with next lineNo "<< maxHeapAtom.lineNo << endl;);
}