#include "stats.h"
#include "global.h"

using namespace std; 
extern StatsDS _gStats; 

void collectStat( uint32_t newFlags){
	
	++ _gStats.Ref;
	
	// find read or write count
	
	if(newFlags	&	READ){
		++_gStats.PageRead ;
		// Collect Read stats
		if(newFlags	&	PAGEHIT){
			++ _gStats.PageReadHit;
			assert( newFlags & BLKHIT);
			assert( !(newFlags & PAGEMISS));
		}
		if(newFlags	&	PAGEMISS)
			++ _gStats.PageReadMiss;
		if(newFlags	&	BLKHIT){
			++ _gStats.BlockWriteHit;
			assert( !(newFlags & BLKMISS) );
		}
		if(newFlags	&	BLKMISS){
			++ _gStats.BlockReadMiss;
			++ _gStats.PageReadMiss;
		}
	}
	else if(newFlags	&	WRITE){
		++_gStats.PageWrite;
		// Collect Read stats
		if(newFlags	&	PAGEHIT){
			++ _gStats.PageWriteHit;
			assert( newFlags & BLKHIT);
			assert( !(newFlags & PAGEMISS));
		}
		if(newFlags	&	PAGEMISS)
			++ _gStats.PageWriteMiss;
		if(newFlags	&	BLKHIT){
			++ _gStats.BlockWriteHit;
			assert( !(newFlags & BLKMISS) );
		}
		if(newFlags	&	BLKMISS){
			++ _gStats.BlockWriteMiss;
			++ _gStats.PageWriteMiss;
		}
		
		if(newFlags	&	EVICT)
			++ _gStats.BlockEvict;
	}
	else{
		cerr<<"Error: Unknown request type in stat collection"<<endl;
		assert(0);
	}
}

void printStats(){
	
	ofstream statStream;
	string fileName("Stats/");
	fileName.append(_gConfiguration.testName);
	fileName.append(".stat");
	statStream.open(fileName, ios::out|ios::app);
	if( ! statStream.good() ){
		cerr<<"Error: can not open stat file: "<<fileName<<endl;
		return;
	}
	
	statStream<<_gConfiguration.testName<<endl;
	Stat * tempStat;
	while( ( tempStat = _gStats.next() ) ){
		statStream<< tempStat->print() <<endl;
	}
	
	statStream.close();
}