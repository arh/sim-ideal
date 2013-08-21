#include <iostream>
#include <time.h>
#include <deque>
#include <stdlib.h>

#include "global.h"
#include "owbp.h"
#include "cpp_framework.h"
#include "configuration.h"
#include "parser.h"
#include "lru_stl.h"
#include "lru_ziqi.h"
#include "lru_dynamic.h"
#include "lru_dynamicB.h"
#include "lru_dynamicC.h"
#include "lru_hotCold.h"
#include "stats.h"
#include "min.h"

using namespace std;

////////////////////////////////////////////////////////////////////////////////
//GLOBALS
////////////////////////////////////////////////////////////////////////////////

int totalSeqEvictedDirtyPages;

int totalNonSeqEvictedDirtyPages;

int totalEvictedCleanPages;

int threshold;

Configuration	_gConfiguration;
bool _gTraceBased = false;
TestCache<uint64_t, cacheAtom> * *_gTestCache; // pointer to each cache class in the hierachy
StatsDS *_gStats;
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

    while(getAndParseMSR(_gConfiguration.traceStream , &newAtom)) {
        ///ziqi: if writeOnly is 1, only insert write cache miss page to cache
        if(_gConfiguration.writeOnly) {
            if(newAtom.flags & WRITE) {
#ifdef REQSIZE
                uint32_t reqSize = newAtom.reqSize;
                newAtom.reqSize = 1;

                //expand large request
                for(uint32_t i = 0 ; i < reqSize ; ++ i) {
                    memTrace.push_back(newAtom);
                    ++ newAtom.fsblkno;
                }

#else
                memTrace.push_back(newAtom);
#endif
            }
        }
        ///ziqi: if writeOnly is 0, insert both read & write cache miss page to cache
        else {
#ifdef REQSIZE
            uint32_t reqSize = newAtom.reqSize;
            newAtom.reqSize = 1;

            //expand large request
            for(uint32_t i = 0 ; i < reqSize ; ++ i) {
                memTrace.push_back(newAtom);
                ++ newAtom.fsblkno;
            }

#else
            memTrace.push_back(newAtom);
#endif
        }

        assert(lineNo < newAtom.lineNo);
        IFDEBUG(lineNo = newAtom.lineNo;);
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
    _gTestCache = new TestCache<uint64_t, cacheAtom>*[_gConfiguration.totalLevels];

    for(int i = 0; i < _gConfiguration.totalLevels ; i++) {
        if(_gConfiguration.GetAlgName(i).compare("pagelru") == 0) {
            _gTestCache[i] = new PageLRUCache<uint64_t, cacheAtom>(cacheAll, _gConfiguration.cacheSize[i], i);
        }
        else
            if(_gConfiguration.GetAlgName(i).compare("ziqilru") == 0) {
                _gTestCache[i] = new ZiqiLRUCache<uint64_t, cacheAtom>(cacheAll, _gConfiguration.cacheSize[i], i);
            }
            else
		if(_gConfiguration.GetAlgName(i).compare("dynamiclru") == 0) {
		    _gTestCache[i] = new DynamicLRUCache<uint64_t, cacheAtom>(cacheAll, _gConfiguration.cacheSize[i], i);
		}
		else
		  if(_gConfiguration.GetAlgName(i).compare("dynamicBlru") == 0) {
		    _gTestCache[i] = new DynamicBLRUCache<uint64_t, cacheAtom>(cacheAll, _gConfiguration.cacheSize[i], i);
		  }
		  else
		  if(_gConfiguration.GetAlgName(i).compare("dynamicClru") == 0) {
		    _gTestCache[i] = new DynamicCLRUCache<uint64_t, cacheAtom>(cacheAll, _gConfiguration.cacheSize[i], i);
		  }
		    else
		      if(_gConfiguration.GetAlgName(i).compare("hotcoldlru") == 0) {
			_gTestCache[i] = new HotColdLRUCache<uint64_t, cacheAtom>(cacheAll, _gConfiguration.cacheSize[i], i);
		      }
		      else
			if(_gConfiguration.GetAlgName(i).compare("pagemin") == 0) {
			    _gTestCache[i] = new PageMinCache(cacheAll, _gConfiguration.cacheSize[i], i);
			}
			else
			    if(_gConfiguration.GetAlgName(i).compare("blockmin") == 0) {
				_gTestCache[i] = new BlockMinCache(cacheAll, _gConfiguration.cacheSize[i], i);
			    }
			    else
				if(_gConfiguration.GetAlgName(i).find("owbp") != string::npos) {
				    _gTestCache[i] = new OwbpCache(cacheAll, _gConfiguration.cacheSize[i], i);
				}
		//esle if //add new policy name and dynamic allocation here
				else {
				    cerr << "Error: UnKnown Algorithm name " << endl;
				    exit(1);
				}
    }

    PRINTV(logfile << "Configuration and setup done" << endl;);
    srand(0);
}
void reportProgress()
{
    static uint64_t totalTraceLines = memTrace.size();
    static int lock = -1;
    int completePercent = ((totalTraceLines - memTrace.size()) * 100) / totalTraceLines ;

    if(completePercent % 10 == 0 && lock != completePercent) {
        lock = completePercent ;
        std::cerr << "\r--> " << completePercent << "% done" << flush;
    }

    if(completePercent == 100)
        std::cerr << endl;
}

/*backup
void recordOutTrace( int level, reqAtom newReq){
	if(_gConfiguration.outTraceStream[level].is_open()){
		_gConfiguration.outTraceStream[level] << newReq.issueTime << "," <<"OutLevel"<<level<<",0,";

		//_gConfiguration.outTraceStream[level] <<"flags: "<< newReq.flags <<" !";

		if(newReq.flags & READ){
			_gConfiguration.outTraceStream[level] << "Read,";
		}
		else
			_gConfiguration.outTraceStream[level] << "Write,";
		//FIXME: check math
		_gConfiguration.outTraceStream[level] << newReq.fsblkno * 512 * 8 <<","<< newReq.reqSize * 512 << endl;

	}
}
*/

///ziqi: this has no use by Jun 19 2013
void recordOutTrace(int level, reqAtom newReq)
{
  /*
    if(_gConfiguration.outTraceStream[level].is_open()) {
        _gConfiguration.outTraceStream[level] << newReq.issueTime << "!";
        //_gConfiguration.outTraceStream[level] <<"flags: "<< newReq.flags <<" !";
        
        if(newReq.flags & READ){
        	_gConfiguration.outTraceStream[level] << "Read,";
        }
        else
        	_gConfiguration.outTraceStream[level] << "Write,";
        
        //FIXME: check math
        _gConfiguration.outTraceStream[level] << newReq.fsblkno << endl;
        //_gConfiguration.outTraceStream[level] << newReq.flags << endl;
    }
  */
}

void runDiskSim()
{
    std::string command = _gConfiguration.diskSimPath;
    command += _gConfiguration.diskSimuExe;
    command += " ";
    command += _gConfiguration.diskSimPath;
    command += _gConfiguration.diskSimParv;
    command += " ";
    command += _gConfiguration.diskSimPath;
    command += _gConfiguration.diskSimOutv;
    command += " ascii ";
    
    //command += _gConfiguration.cache2diskPipeFileName;
    ///ziqi: the line above is by Alireza. I use diskSimInputTraceName to denote the DiskSim input trace file name
    command += _gConfiguration.diskSimInputTraceName;
    
    command += " 0";
    PRINTV(logfile << "Running Disk Simulator with following command:" << endl;);
    PRINTV(logfile << command << endl;);
    system(command.c_str());
}

void runSeqLengthAnalysis()
{
    std::string command = _gConfiguration.analysisAppPath;
    command += _gConfiguration.analysisAppExe;
    command += " ";
    command += _gConfiguration.diskSimInputTraceName;
    command += " ";
    command += "analyzed-"+_gConfiguration.diskSimInputTraceName;
    
    PRINTV(logfile << "Running Seq Length Analysis App with following command:" << endl;);
    PRINTV(logfile << command << endl;);
    system(command.c_str());
    
}

void RunBenchmark(deque<reqAtom> & memTrace)
{
    PRINTV(logfile << "Start benchmarking" << endl;);

    while(! memTrace.empty()) {
        uint32_t newFlags = 0;
        reqAtom newReq = memTrace.front();
        cacheAtom newCacheAtom(newReq);

        //access hierachy from top layer
        for(int i = 0 ; i < _gConfiguration.totalLevels ; i++) {
            newFlags = _gTestCache[i]->access(newReq.fsblkno, newCacheAtom, newReq.flags);
            collectStat(i, newFlags);

            if(newFlags & PAGEHIT)
                break; // no need to check further down in the hierachy

            recordOutTrace(i, newReq);
            newFlags = 0; // reset flag
        }

        memTrace.pop_front();
        reportProgress();
    }

    if(! _gConfiguration.diskSimuExe.empty()) {
        PRINTV(logfile << "Multi-level Cache Simulation is Done, Start Timing Simulation with Disk simulator" << endl;);
        runDiskSim();
    }
    
    if(! _gConfiguration.analysisAppExe.empty()) {
        PRINTV(logfile << "Timing Simulation is Done, Start Sequential Length Analysis" << endl;);
        runSeqLengthAnalysis();
    }

    PRINTV(logfile << "Benchmarking Done" << endl;);
}

int main(int argc, char **argv)
{
    totalEvictedCleanPages = 0;
    totalSeqEvictedDirtyPages = 0;
    totalNonSeqEvictedDirtyPages = 0;
    //read benchmark configuration
    Initialize(argc, argv, memTrace);   
    
    if(_gConfiguration.GetAlgName(0).compare("dynamiclru") == 0 
      ||_gConfiguration.GetAlgName(0).compare("dynamicBlru") == 0
      ||_gConfiguration.GetAlgName(0).compare("dynamicClru") == 0
      ||_gConfiguration.GetAlgName(0).compare("hotcoldlru") == 0) 
    {
      threshold = 1;
    }
    else
      threshold = _gConfiguration.seqThreshold;
    
    RunBenchmark(memTrace); // send reference memTrace
    ExitNow(0);
}
