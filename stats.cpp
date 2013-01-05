#include "stats.h"
#include "global.h"

using namespace std; 
extern StatsDS _gStats; 

void collectStat( uint32_t newFlags){
	
	++ _gStats.Ref;

	if(newFlags	&	PAGEHIT){
		++ _gStats.PageHit;
		assert( newFlags & BLKHIT);
		assert( !(newFlags & PAGEMISS));
	}
	if(newFlags	&	PAGEMISS)
		++ _gStats.PageMiss;
	if(newFlags	&	BLKHIT){
		++ _gStats.BlockHit;
		assert( !(newFlags & BLKMISS) );
	}
	if(newFlags	&	BLKMISS)
		++ _gStats.BlockMiss;
	if(newFlags	&	EVICT)
		++ _gStats.BlockEvict;
}

void printStats(){
	
	ofstream statStream;
	string fileName(_gConfiguration.traceName);
	fileName.append(".stat");
	statStream.open(fileName);
	if( ! statStream.good() ){
		cerr<<"can not open stat file: "<<fileName<<endl;
		return;
	}
	
	
	Stat * tempStat;
	while( ( tempStat = _gStats.next() ) ){
		statStream<< tempStat->print() <<endl;
	}
	
	statStream.close();
}