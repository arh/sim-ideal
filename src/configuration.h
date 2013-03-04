#ifndef __CONFIGURATION__
#define __CONFIGURATION__

#include <string>
#include <ctime>
#include <cassert>
#include <iostream>
#include <fstream>
#include <string.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/info_parser.hpp>
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
	std::string * policyName;
	char*   testName;
	uint64_t * cacheSize;  // in pages 
	uint64_t fsblkSize;
	uint64_t *cachePageSize;
	uint64_t *cacheBlkSize;
	uint32_t *ssd2fsblkRatio;
	uint64_t maxLineNo;
	uint32_t futureWindowSize;
	uint64_t *birdHist;
	uint64_t *pirdHist;
	int 	totalLevels;

	void initHist(){
		assert(futureWindowSize);
		for(unsigned i=0 ; i < futureWindowSize ; ++i ){
			birdHist[i]=0;
			pirdHist[i]=0;
		}
	}


	bool read(int argc, char **argv) ;

	std::string GetAlgName( int i ) {
		if( policyName[i].find("owbp") != std::string::npos){
			std::ostringstream convert;
			convert << futureWindowSize/cacheSize[i] ;
			return std::string(policyName[i] +  "-" + (convert.str()) + ("x") );
		}
		else
			return policyName[i];
	}

	std::string PrintTestName() {
		return std::string(testName);
	}

	std::string GetTraceName() {
		return std::string(traceName);
	}

	Configuration() {
		traceName = NULL;
		policyName = NULL;
		cacheSize = NULL;
		totalLevels = 0; 
		testName = 0;
// 		ssdblkSize = 16384 ; 
		maxLineNo = 0;
		
		birdHist = NULL;
		pirdHist = NULL;

		
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
		delete [] cacheSize;
		delete [] policyName;
		delete [] cacheBlkSize;
		delete [] cachePageSize;
		delete [] ssd2fsblkRatio;
	}

	private:
	void allocateArrays(int totalLevels);
	uint64_t myString2intConverter(std::string temp);
};



#endif
