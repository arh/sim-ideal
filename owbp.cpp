#include "owbp.h"

extern deque<reqAtom> memTrace;

void OwbpCacheBlock::updateMetaDataOnPageInsert(const cacheAtom value)
{
	//check if firstValue is in the curr position of memTrace
	assert(memTrace.size()); 
	uint32_t currLine = value.getLineNo();
	assert( currLine == memTrace.front().lineNo );
	assert( meta.BlkID == value.getSsdblkno() );
	bool assignedFirstBlkRef = false; 
	bool pageAccessInfutureWindow = false;
	// find out min distance to future ref to the same block
	// find out if firstValue page is cold ?
	unordered_set<uint64_t> uniqSet; 
	uniqSet.clear();
	
	deque<reqAtom>::iterator it = memTrace.begin(); // iterate over the memTrace
	++ it; //skip over currLine
	
// 	pair<deque<reqAtom>::iterator,bool> ret;
	
	for( ; it != memTrace.end() ; ++ it ){
		assert(  currLine < it->lineNo );
		
		/*ret = */uniqSet.insert(it->fsblkno); // insert page ID in the uniqSet
		if(it->ssdblkno == meta.BlkID ){
			if(assignedFirstBlkRef == false){
				assert(uniqSet.size());
				meta.distance= uniqSet.size() ;
				assignedFirstBlkRef = true;
			}
			if(it->fsblkno == value.getFsblkno()){
				pageAccessInfutureWindow = true ;
			}
		}
		
		
		if(uniqSet.size() > _gConfiguration.futureWindowSize ){ // hopefully size() complexity is O(1)
		
			break;
		}
	}
	
	if(pageAccessInfutureWindow == false){
		++ meta.coldPageCounter;
	}
	
	if(assignedFirstBlkRef == false){
		assert(pageAccessInfutureWindow == false); 
		meta.distance = INF; 
	}
	
	assert( meta.distance ); 
	
}

uint32_t OwbpCacheBlock::readPage(cacheAtom value)
{
	PageSetType::iterator it =  pageSet.find(value);
	if( it == pageSet.end() ){
		return PAGEMISS;
	}
	else{
		return PAGEHIT;
	}
}

uint32_t OwbpCacheBlock::writePage(cacheAtom value)
{
	assert(memTrace.front().lineNo == value.getLineNo() ); //make sure that write comes with the same sequence recorded in the queue
	
	
	// update distance and coldness value
	updateMetaDataOnPageInsert(value) ;
	
	pair < PageSetType::iterator , bool > ret =  pageSet.insert(value);
	if( ret.second == true ){
		assert( ret.first != pageSet.end() );
		return PAGEMISS;
	}
	else{
		assert( ret.first != pageSet.end() ) ;
		size_t tempSize;
		IFDEBUG( tempSize = pageSet.size(); );
		pageSet.erase(ret.first);
		
		IFDEBUG( assert( (tempSize -1 ) == pageSet.size() ); );
		
		pageSet.insert(value);
		IFDEBUG( assert( tempSize == pageSet.size() ); );
		
		return PAGEHIT;
	}
}

void OwbpCache::insertNewBlk(cacheAtom& value){
	PRINTV(logfile << "Insert new block on page ID miss: " << value.getFsblkno() << endl;);
	
	assert(currSize <= _capacity); //evict call happen in access function
	uint64_t currSsdBlkNo = value.getSsdblkno();
	
	assert(value.getLineNo() == memTrace.front().lineNo); //check position 

	pair< map<uint64_t,OwbpCacheBlock>::iterator, bool> ret;
	
	
	OwbpCacheBlock tempBlock(value); //constructor will update block metadata (coldness, distance ) as well 
	
	//insert in coldheap or victimPull
	// make sure it is not already there
	assert( coldHeap.find(tempBlock.meta) == coldHeap.end() ); //this assertaion may fail because there might be multiple block with the same condition
	assert( victimPull.find(value.getSsdblkno() ) == victimPull.end() );
	
	if(tempBlock.getMinFutureDist() == INF ){
		victimPull.insert(value.getSsdblkno());
		tempBlock.coldHeapIt = coldHeap.end();
	}
	else{
		ColdHeapIt it;
		it = coldHeap.insert(tempBlock.meta);
		tempBlock.coldHeapIt =  it;
		
	}
	
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
			
			if(currSize >= _capacity ){
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
			
			//insert page first in the block then if it found page miss, it will check to trigger evict()

			//PAGEMISS or hit to status is return value of this function
			IFDEBUG( valid_pages = blkit->second.getPageSetSize(); );
			status |= blkit->second.writePage(value); //insert new page and update block metabata
			IFDEBUG( assert( status&PAGEHIT ? valid_pages == blkit->second.getPageSetSize()  :valid_pages + 1 == blkit->second.getPageSetSize() ););
			
			//update coldHeap and victimPull
			ColdHeapIt it = blkit->second.coldHeapIt;
			
			if( it  == coldHeap.end() ){
				// block is in victimPull
				victimIt itV = victimPull.find( blkit->second.getBlkID() );
				assert( itV != victimPull.end() ); 
				if(blkit->second.getMinFutureDist() != INF ){
					// move block from victimpull to coldheap
					victimPull.erase(blkit->second.getBlkID());
					ColdHeapIt itRef;
					itRef = coldHeap.insert(blkit->second.meta);
					blkit->second.coldHeapIt =  itRef;
				}
			}
			else{
				// block is in coldheap
				assert( victimPull.find(blkit->second.getBlkID() ) == victimPull.end() );
				// update coldheap
				coldHeap.erase(it);
				if(blkit->second.getMinFutureDist() == INF ){
					victimPull.insert(value.getSsdblkno());
					blkit->second.coldHeapIt = coldHeap.end();
				}
				else{
					ColdHeapIt itRef;
					itRef = coldHeap.insert(blkit->second.meta);
					blkit->second.coldHeapIt =  itRef;
					
				}
				
			}
			
			if( status & PAGEMISS){
				++ currSize;
				if(currSize >= _capacity ){
					evict();
					status |= EVICT;
				}
			}
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
	assert( currSize == _capacity );
	uint64_t victimBlkID;
	if( victimPull.size() != 0 ){
		victimIt itV = victimPull.begin() ;
		victimBlkID = *itV;
		victimPull.erase(itV);
	}
	else{
		assert( coldHeap.size() != 0 );
		ColdHeapIt itH = coldHeap.begin(); // assume there is no douplicated entry with the same condition in the multiset
		victimBlkID = itH->BlkID;
		coldHeap.erase(itH);
	}
	map< uint64_t, OwbpCacheBlock >::iterator itOw = blkID_2_DS.find(victimBlkID);
	assert( itOw != blkID_2_DS.end() );
	
	size_t victimSize = itOw->second.getPageSetSize();
	currSize -= victimSize;
	
	//remove from main table
	blkID_2_DS.erase(itOw);
// 	PRINTV(logfile<<" evicting victim block "<< maxHeapAtom.key <<" with next lineNo "<< maxHeapAtom.lineNo << endl;);
}


