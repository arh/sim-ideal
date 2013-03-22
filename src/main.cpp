#include <iostream>
#include <time.h>
#include <deque>

#include "global.h"
#include "owbp.h"
#include "cpp_framework.h"
#include "configuration.h"
#include "parser.h"
#include "lru_stl.h"
#include "stats.h"
#include "min.h"

using namespace std;

////////////////////////////////////////////////////////////////////////////////
//GLOBALS
////////////////////////////////////////////////////////////////////////////////

Configuration	_gConfiguration;
bool _gTraceBased = false; 
TestCache<uint64_t,cacheAtom> ** _gTestCache; // pointer to each cache class in the hierachy 
StatsDS * _gStats;
deque<reqAtom> memTrace; // in memory trace file


void	readTrace(deque<reqAtom> & memTrace)
{
	assert(_gTraceBased); // read from stdin is not implemented
	_gConfiguration.traceStream.open(_gConfiguration.traceName, ifstream::in);

	if(! _gConfiguration.traceStream.good()) {
		PRINT(cout << " Error: Can not open trace file : " << _gConfiguration.traceName << endl;);
		ExitNow(1);
	}
	reqAtom newAtom;
	uint32_t lineNo = 0;
	while(getAndParseMSR(_gConfiguration.traceStream , &newAtom)){
		//drop all reads if simulation setup is writeOnly
		if(newAtom.flags & WRITE && _gConfiguration.writeOnly){
#ifdef REQSIZE
			uint32_t reqSize= newAtom.reqSize; 
			newAtom.reqSize = 1;
			//expand large request
			for( uint32_t i=0 ; i < reqSize ; ++ i) {
				memTrace.push_back(newAtom);
				++ newAtom.fsblkno; 
			}
#else
			memTrace.push_back(newAtom);
#endif
		}
		assert(lineNo < newAtom.lineNo ); 
		IFDEBUG( lineNo = newAtom.lineNo;  );
		newAtom.clear();
	}
	
	_gConfiguration.traceStream.close();
	
}

void	Initialize(int argc, char **argv, deque<reqAtom> & memTrace)
{
	if(!_gConfiguration.read(argc, argv)) {
		cerr << "USAGE: <TraceFilename> <CfgFileName> <TestName>" << endl;
		exit(-1);
	}

	readTrace(memTrace);
	assert(memTrace.size() != 0);
	
	//Allocate StatDs
	_gStats = new StatsDS[_gConfiguration.totalLevels]; 

	//Allocate hierarchy 
	_gTestCache = new TestCache<uint64_t,cacheAtom>*[_gConfiguration.totalLevels];
	for( int i = 0; i < _gConfiguration.totalLevels ; i++){
		if( _gConfiguration.GetAlgName(i).compare("pagelru") == 0 ){
			_gTestCache[i] = new PageLRUCache<uint64_t,cacheAtom>(cacheAll, _gConfiguration.cacheSize[i], i);
		}
		else if ( _gConfiguration.GetAlgName(i).compare("pagemin") == 0	 ){
			_gTestCache[i] = new PageMinCache(cacheAll, _gConfiguration.cacheSize[i], i);
		}
		else if ( _gConfiguration.GetAlgName(i).compare("blockmin") == 0	 ){
			_gTestCache[i] = new BlockMinCache(cacheAll, _gConfiguration.cacheSize[i], i);
		}
		else if ( _gConfiguration.GetAlgName(i).find("owbp") != string::npos	 ){
			_gTestCache[i] = new OwbpCache(cacheAll, _gConfiguration.cacheSize[i], i);
		}
		//esle if //add new policy name and dynamic allocation here
		else{
			cerr<< "Error: UnKnown Algorithm name " <<endl;
			exit(1);
		}
	}
	PRINTV (logfile << "Configuration and setup done" << endl;);
	srand(0);
}
void reportProgress(){

	static uint64_t totalTraceLines = memTrace.size();
	static int lock= -1; 
	int completePercent = ((totalTraceLines-memTrace.size())*100)/totalTraceLines ;
	if(completePercent%10 == 0 && lock != completePercent ){
		lock = completePercent ;
		std::cerr<<"\r--> "<<completePercent<<"% done"<<flush;
	}
	if(completePercent == 100 )
		std::cerr<<endl;
}
void recordOutTrace( int level, reqAtom newReq){
	if(_gConfiguration.outTraceStream[level].is_open()){
		_gConfiguration.outTraceStream[level] << newReq.issueTime << "," <<"OutLevel"<<level<<",0,";
		if(newReq.flags & READ){
			_gConfiguration.outTraceStream[level] << "Read,";
		}
		else
			_gConfiguration.outTraceStream[level] << "Write,";
		//FIXME: check math
		_gConfiguration.outTraceStream[level] << newReq.fsblkno * 512 * 8 <<","<< newReq.reqSize * 512 << endl;
		
	}
}
void RunBenchmark( deque<reqAtom> & memTrace){
	PRINTV (logfile << "Start benchmarking" << endl;);
	while( ! memTrace.empty() ){
		uint32_t newFlags = 0;
		reqAtom newReq = memTrace.front();	
		cacheAtom newCacheAtom(newReq);
		//access hierachy from top layer
		for( int i=0 ; i < _gConfiguration.totalLevels ; i++){
			newFlags = _gTestCache[i]->access(newReq.fsblkno,newCacheAtom,newReq.flags);
			collectStat(i,newFlags);
			if(newFlags & PAGEHIT)
				break; // no need to check further down in the hierachy
			recordOutTrace(i,newReq);
			newFlags = 0; // reset flag
		}
		memTrace.pop_front();
		reportProgress();
	}
	PRINTV (logfile << "Benchmarking Done" << endl;);
}

int main(int argc, char **argv)
{

	//read benchmark configuration
	Initialize(argc, argv,memTrace);
 	RunBenchmark(memTrace); // send reference memTrace
	ExitNow(0);
}
