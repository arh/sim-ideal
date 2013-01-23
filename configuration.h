#ifndef __CONFIGURATION__
#define __CONFIGURATION__

#include <string>
#include <ctime>
#include <assert.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include "cpp_framework.h"



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



	bool read(int argc, char **argv) {

		if(argc < 5)
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
		futureWindowSize = L1cacheSize;
		return true;
	}

	std::string GetAlgName() {
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
		ssdblkSize = 512 * 1024 ;
		ssd2fsblkRatio = ssdblkSize / fsblkSize;
		maxLineNo = 0;
		logStream.open("log.txt",std::ios::trunc);
		
		//print start time
		time_t now = time(0);
		tm* localtm = localtime(&now);
		logStream<<"Start Logging at "<< asctime(localtm) <<std::endl;
	}

	~Configuration() {
		traceStream.close();
		
		//print end time
		time_t now = time(0);
		tm* localtm = localtime(&now);
		logStream<<"End Logging at "<< asctime(localtm) <<std::endl;
		logStream.close();
	}

};



#endif
