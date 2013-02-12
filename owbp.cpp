#include "owbp.h"

extern deque<reqAtom> memTrace;

uint32_t OwbpCacheBlock::updateMetaDataOnPageInsert(const cacheAtom value)
{
	//check if firstValue is in the curr position of memTrace
	assert(memTrace.size()); 
	uint32_t status=0;
	uint32_t currLine = value.getLineNo();
	assert( currLine == memTrace.front().lineNo );
	assert( meta.BlkID == value.getSsdblkno() );
	bool assignedFirstBlkRef = false; 
	bool pageAccessInfutureWindow = false;
	// find out min distance to future ref to the same block
	// find out if firstValue page is cold ?
	set<uint64_t> uniqSet; 
	uniqSet.clear();
	
	deque<reqAtom>::iterator it = memTrace.begin(); // iterate over the memTrace
	++ it; //skip over currLine
	
	for( ; it != memTrace.end() ; ++ it ){
/*		pair<unordered_set<uint64_t>::iterator, bool> ret; 
		ret =*/ uniqSet.insert((uint64_t) it->fsblkno); // insert page ID in the uniqSet
// 		if(ret.second == false){
// 			assert(0);
// 		}
		assert(  currLine <= it->lineNo );
		if(it->ssdblkno == meta.BlkID ){
			if(assignedFirstBlkRef == false){
				assert(uniqSet.size());
				meta.distance= uniqSet.size() ;
				assignedFirstBlkRef = true;
			}
			if(it->fsblkno == value.getFsblkno() ){
				pageAccessInfutureWindow = true;
			}
		}
		
		if(uniqSet.size() >= ( _gConfiguration.futureWindowSize - 1) ){ // -1 because currBlock is excluded from uniqSet, hopefully size() complexity is O(1)
		
			break;
		}
	}
	
	if(pageAccessInfutureWindow == false){
		pair<set<uint64_t>::iterator,bool> ret; 
		ret = coldPageSet.insert(value.getFsblkno());
		if( ret.second == false ){
			PRINTV( logfile<<"\tpage "<< value.getFsblkno() <<" was already cold" << endl; );
			status = COLD2COLD;
		}
	}
	else{ // page is not cold 
		set<uint64_t>::iterator it;
		it =  coldPageSet.find(value.getFsblkno());
		if(it != coldPageSet.end() ){ // page is in the block coldset
			PRINTV( logfile<<"\tpage "<< value.getFsblkno() <<" was cold and now is convert to hot" << endl; );
			coldPageSet.erase(it);
			status = COLD2HOT;
		}
	}
	meta.coldPageCounter = coldPageSet.size();
	
	if(assignedFirstBlkRef == false){
		assert(pageAccessInfutureWindow == false); 
		meta.distance = INF; 
	}
	
	assert( meta.distance ); 
	return status; 
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
	uint32_t status=0;
	
	// update distance and coldness value
	
	pair < PageSetType::iterator , bool > ret =  pageSet.insert(value);
	if( ret.second == true ){
		assert( ret.first != pageSet.end() );
		status = updateMetaDataOnPageInsert(value) ;
		return status | PAGEMISS;
	}
	else{
		assert( ret.first != pageSet.end() ) ;
		size_t tempSize;
		
		IFDEBUG( tempSize = pageSet.size(); );
		pageSet.erase(ret.first);
		IFDEBUG( assert( (tempSize -1 ) == pageSet.size() ); );
		pageSet.insert(value);
		IFDEBUG( assert( tempSize == pageSet.size() ); );

		status = updateMetaDataOnPageInsert(value) ;
		
		return status | PAGEHIT;
	}
}

uint32_t OwbpCacheBlock::findPage(cacheAtom value)
{
	
	PageSetType::iterator it =  pageSet.find(value);
	if( it == pageSet.end() ){
		return PAGEMISS;
	}
	else{
		return PAGEHIT;
	}
}


void OwbpCache::insertNewBlk(cacheAtom& value){
	PRINTV(logfile << "\tInsert new block on page miss on PageID: " << value.getFsblkno() << endl;);
	
	assert(currSize < _capacity); //evict call happen in access function
	uint64_t currSsdBlkNo = value.getSsdblkno();
	
	assert(value.getLineNo() == memTrace.front().lineNo); //check position 

	pair< map<uint64_t,OwbpCacheBlock>::iterator, bool> ret;
	
	
	OwbpCacheBlock tempBlock(value); //constructor will update block metadata (coldness, distance ) as well 
	
	//insert in coldheap or victimPull
	// make sure it is not already there
// 	assert( coldHeap.find(tempBlock.meta) == coldHeap.end() ); //this assertaion may fail because there might be multiple block with the same condition
	// failure of previous assert observerd, 
	assert( victimPull.find(value.getSsdblkno() ) == victimPull.end() );
	
	if(tempBlock.getMinFutureDist() == INF ){
		victimPull.insert(value.getSsdblkno());
		tempBlock.coldHeapIt = coldHeap.end();
	}
	else{
		ColdHeapIt it;
		it = coldHeap.insert(tempBlock.meta);
		tempBlock.coldHeapIt =  it;
		
		//debug
		ColdHeapIt itDebug;
		itDebug = coldHeap.find(tempBlock.meta);
// 		assert(it->coldPageCounter !=0);
		assert(itDebug->coldPageCounter == it->coldPageCounter );
		
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
			
			if(currSize == _capacity ){
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
			// loock up for pagehit or page miss
			status |= blkit->second.findPage(value);
			if(status & PAGEMISS){

				if(currSize == _capacity ){
					evict();
					status |= EVICT;
				}
				++ currSize;
				
				uint32_t debugStatus=0;
				assert( valid_pages = blkit->second.getPageSetSize() );
				debugStatus = blkit->second.writePage(value); //insert new page and update block metabata
				assert(valid_pages + 1 == blkit->second.getPageSetSize() );
				assert(debugStatus & PAGEMISS);
				status |= debugStatus; 
				
				
			}
			else{
				assert(status & PAGEHIT);
				uint32_t debugStatus=0;
				assert( valid_pages = blkit->second.getPageSetSize() ); // read valid pages for debug 
				debugStatus = blkit->second.writePage(value); //insert new page and update block metabata
				assert( valid_pages == blkit->second.getPageSetSize() );
				assert(debugStatus & PAGEHIT);
				status |= debugStatus; 
				
			}
			
			
			//update block metadata in coldHeap and victimPull
			ColdHeapIt it = blkit->second.coldHeapIt;
			
			if( it  == coldHeap.end() ){
				// block is in victimPull
				victimIt itV = victimPull.find( blkit->second.getBlkID() );
				assert( itV != victimPull.end() ); // make sure that block is in victimpull 
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
		ColdHeapIt itH = -- coldHeap.end(); // assume there is no douplicated entry with the same condition in the multiset, this assumption is no longer valid, but the code funcionality is validated
		victimBlkID = itH->BlkID;
		coldHeap.erase(itH);
	}
	PRINTV(logfile<<"\tevicting victim block ID"<< victimBlkID <<endl;);
	map< uint64_t, OwbpCacheBlock >::iterator itOw = blkID_2_DS.find(victimBlkID);
	assert( itOw != blkID_2_DS.end() );
	
	size_t victimSize = itOw->second.getPageSetSize();
	assert(victimSize); 
	currSize -= victimSize;
	
	//remove from main table
	blkID_2_DS.erase(itOw);
}


