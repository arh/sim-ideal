#include <iostream>
#include <time.h>
#include <queue>
#include "global.h"
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
TestCache<uint64_t,cacheAtom> * _gTestCache;
StatsDS _gStats; 


void	readTrace(queue<reqAtom> & memTrace)
{
	assert(_gTraceBased); // read from stdin is not implemented
	_gConfiguration.traceStream.open(_gConfiguration.traceName, ifstream::in);

	if(! _gConfiguration.traceStream.good()) {
		PRINT(cout << " Error: Can not open trace file : " << _gConfiguration.traceName << endl;);
		ExitNow(1);
	}
	reqAtom newAtom;
	while(getAndParseMSR(_gConfiguration.traceStream , &newAtom)){
		memTrace.push(newAtom);
		newAtom.clear();
	}
	
	_gConfiguration.traceStream.close();
	
}

void	Initialize(int argc, char **argv, queue<reqAtom> & memTrace)
{
	if(!_gConfiguration.read(argc, argv)) {
		cerr << "USAGE: <tracefilename> <AlgName> <TestName> <L1Size>" << endl;
		exit(-1);
	}

	readTrace(memTrace);
	assert(memTrace.size() != 0);
// 	lru_stl< uint64_t , cacheAtom> LRUCache(cacheAll, _gConfiguration.L1cacheSize);
	if( _gConfiguration.GetAlgName().compare("pagelru") == 0 ){
		_gTestCache = new PageLRUCache<uint64_t,cacheAtom>(cacheAll, _gConfiguration.L1cacheSize);
	}
	else if ( _gConfiguration.GetAlgName().compare("pagemin") == 0	 ){
		_gTestCache = new PageMinCache(cacheAll, _gConfiguration.L1cacheSize);
	}
	else if ( _gConfiguration.GetAlgName().compare("blockmin") == 0	 ){
		_gTestCache = new BlockMinCache(cacheAll, _gConfiguration.L1cacheSize);
	}
	else{
		cerr<< "Error: UnKnown Algorithm name " <<endl;
	}
	PRINTV (logfile << "Configuration and setup done" << endl;);
	srand(0);
}

void RunBenchmark( queue<reqAtom> & memTrace){
	PRINTV (logfile << "Start benchmarking" << endl;);
	while( ! memTrace.empty() ){
		reqAtom newReq = memTrace.front();
		memTrace.pop();
		
		unsigned offset=0;
		while(newReq.reqSize){
			reqAtom tempReq = newReq;
			tempReq.reqSize=1;
			tempReq.fsblkno+=offset;
			tempReq.ssdblkno= tempReq.fsblkno/_gConfiguration.ssd2fsblkRatio;
			cacheAtom newCacheAtom(tempReq);
			uint32_t newFlags = 0;
			//cach access
			newFlags = _gTestCache->access(tempReq.fsblkno,newCacheAtom,tempReq.flags);
			collectStat(newFlags);
			newCacheAtom.clear();
			tempReq.clear();
			++ offset;
			-- newReq.reqSize;
		}
	}
	PRINTV (logfile << "Benchmarking Done" << endl;);
}

int main(int argc, char **argv)
{
	queue<reqAtom> memTrace; // in memory trace file
	//read benchmark configuration
	Initialize(argc, argv,memTrace);
 	RunBenchmark(memTrace); // send reference memTrace
	ExitNow(0);
}
