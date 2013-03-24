#include <sys/types.h>
#include <sys/stat.h>
#include "stats.h"
#include "global.h"

using namespace std; 
extern StatsDS *  _gStats; 

void collectStat( int level, uint32_t newFlags){
	
	++ _gStats[level].Ref;
	
	// find read or write count
	
	if(newFlags	&	READ){
		++_gStats[level].PageRead ;
		// Collect Read stats
		if(newFlags	&	PAGEHIT){
			++ _gStats[level].PageReadHit;
			assert( newFlags & BLKHIT);
			assert( !(newFlags & PAGEMISS));
		}
		if(newFlags	&	PAGEMISS)
			++ _gStats[level].PageReadMiss;
		if(newFlags	&	BLKHIT){
			++ _gStats[level].BlockWriteHit;
			assert( !(newFlags & BLKMISS) );
		}
		if(newFlags	&	BLKMISS){
			++ _gStats[level].BlockReadMiss;
			++ _gStats[level].PageReadMiss;
		}
	}
	else if(newFlags	&	WRITE){
		++_gStats[level].PageWrite;
		// Collect Read stats
		if(newFlags	&	PAGEHIT){
			++ _gStats[level].PageWriteHit;
			assert( newFlags & BLKHIT);
			assert( !(newFlags & PAGEMISS));
		}
		
		if(newFlags	&	BLKHIT){
			++ _gStats[level].BlockWriteHit;
			assert( !(newFlags & BLKMISS) );
			if(newFlags	&	PAGEMISS)
				++ _gStats[level].PageWriteMiss;
		}
		if(newFlags	&	BLKMISS){
			assert( !(newFlags & BLKHIT) );
			++ _gStats[level].BlockWriteMiss;
			++ _gStats[level].PageWriteMiss;
		}
		if(newFlags	&	EVICT)
			++ _gStats[level].BlockEvict;
		
		if(newFlags	&	PAGEMISS && ! (newFlags	&BLKHIT ) && !(newFlags	&BLKMISS) ) // for page based algorithm
			++ _gStats[level].PageWriteMiss;
		
		if( newFlags & COLD2COLD ){
			++ _gStats[level].Cold2Cold;
			assert( ! (newFlags & COLD2HOT) ); 
		}
		if(newFlags & COLD2HOT)
			++ _gStats[level].Cold2Hot; 
			
	}
	else{
		cerr<<"Error: Unknown request type in stat collection"<<endl;
		assert(0);
	}
}

// print histograms
void printHist(){
	//TODO: print stat for each cache in hierarchy
	
	int i=0; 
	ofstream pirdStream,birdStream;
	string pirdName("Stats/");
	pirdName.append(_gConfiguration.testName);
	pirdName.append("-");
	pirdName.append(_gConfiguration.GetAlgName(i));
	string birdName(pirdName);
	pirdName.append(".PIRD");
	birdName.append(".BIRD");
	
	pirdStream.open(pirdName, ios::out|ios::trunc);
	if( ! pirdStream.good() ){
		cerr<<"Error: can not open PIRD file: "<<pirdName<<endl;
		return;
	}
	birdStream.open(birdName, ios::out|ios::trunc);
	if( ! birdStream.good() ){
		cerr<<"Error: can not open BIRD file: "<<birdName<<endl;
		return;
	}
	
	for(unsigned i = 0; i < _gConfiguration.futureWindowSize  ; ++i ){
		pirdStream<<i<<"\t"<<_gConfiguration.pirdHist[i]<<endl;
		birdStream<<i<<"\t"<<_gConfiguration.birdHist[i]<<endl;
	}
	pirdStream.close();
	birdStream.close();
}

//print stats
void printStats(){

	ofstream statStream;
	mkdir("Stats", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
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
	
	//print stat results for each level 
	for( int i=0 ; i < _gConfiguration.totalLevels ; i++ ){
		statStream << "Level "<<i+1<<",\t"<<_gConfiguration.GetAlgName(i)<<endl; 
		while( ( tempStat = _gStats[i].next() ) ){
			statStream<< tempStat->print() <<endl;
		}
		statStream<<endl;
	}
	statStream.close();

	IFHIST(printHist(););
	
}