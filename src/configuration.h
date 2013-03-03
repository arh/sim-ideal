#ifndef __CONFIGURATION__
#define __CONFIGURATION__

#include <string>
#include <ctime>
#include <cassert>
#include <iostream>
#include <fstream>
#include <string.h>
#include "cpp_framework.h"

#ifdef HIST
#define IFHIST(X)	do { X	}while(false)
#else
#define IFHIST(X)
#endif


extern bool _gTraceBased;


class Configuration
{
public:
	char* traceName;
	std::ifstream traceStream;
	std::ofstream logStream; 
	char*	algName;
	char*   testName;
	uint64_t L1cacheSize; // in page 
	uint64_t fsblkSize;
	uint64_t ssdblkSize;
	uint32_t ssd2fsblkRatio;
	uint64_t maxLineNo;
	uint32_t futureWindowSize;
	uint64_t *birdHist;
	uint64_t *pirdHist;

	void initHist(){
		assert(futureWindowSize);
		for(unsigned i=0 ; i < futureWindowSize ; ++i ){
			birdHist[i]=0;
			pirdHist[i]=0;
		}
	}


	bool read(int argc, char **argv) {

		if(argc < 6)
			return false;

		int curr_arg = 1;
		traceName = argv[curr_arg++];

		if(GetTraceName().compare("stdin") != 0) {
			_gTraceBased = true;
		}
		else{
			std::cerr<<" Error: read from stdin is not implemented"<<std::endl;
			exit(1);
		}
		algName 	= argv[curr_arg++];
		testName	= argv[curr_arg++];
		L1cacheSize = CMDR::Integer::parseInt(argv[curr_arg++]);
		futureWindowSize = CMDR::Integer::parseInt(argv[curr_arg++]);
		IFHIST(birdHist = new uint64_t[futureWindowSize];);
		IFHIST(pirdHist = new uint64_t[futureWindowSize];);
		IFHIST(initHist(););
		return true;
	}

	std::string GetAlgName() {
		if( strcmp(algName,"owbp") == 0){
			std::ostringstream convert;
			convert << futureWindowSize/L1cacheSize ;
			return std::string(algName).append("-").append(convert.str()).append("x");
		}
		else
			return std::string(algName);
	}

	std::string PrintTestName() {
		return std::string(testName);
	}

	std::string GetTraceName() {
		return std::string(traceName);
	}

	Configuration() {
		traceName = NULL;
		algName = NULL;
		testName = 0;
		L1cacheSize = 0;
		fsblkSize = 4096;
		ssdblkSize = 256 * 1024 ; //64 page per ssd block
// 		ssdblkSize = 16384 ; 
		ssd2fsblkRatio = ssdblkSize / fsblkSize;
		maxLineNo = 0;
		logStream.open("log.txt",std::ios::trunc);
		birdHist = NULL;
		pirdHist = NULL;
		//print start time
		time_t now = time(0);
		tm* localtm = localtime(&now);
		logStream<<"Start Logging at "<< asctime(localtm) <<std::endl;
	}

	~Configuration() {
		
		IFHIST(delete pirdHist;);
		IFHIST(delete birdHist;);
		traceStream.close();
		
		//print end time
		time_t now = time(0);
		tm* localtm = localtime(&now);
		logStream<<"End Logging at "<< asctime(localtm) <<std::endl;
		logStream.close();
	}

};



#endif
