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
    char *traceName;
    std::ifstream traceStream;
    std::ofstream logStream;
    std::ofstream *outTraceStream;
    std::string *policyName;
    char   *testName;
    uint64_t *cacheSize;   // in pages
    uint64_t fsblkSize;
    uint64_t *cachePageSize;
    uint64_t *cacheBlkSize;
    uint32_t *ssd2fsblkRatio;
    uint64_t maxLineNo;
    uint32_t futureWindowSize;
    uint64_t *birdHist;
    uint64_t *pirdHist;
    int 	totalLevels;
    bool writeOnly;

    void initHist();
    bool read(int argc, char **argv) ;
    Configuration();
    ~Configuration();

    inline std::string GetAlgName(int i) {
        if(policyName[i].find("owbp") != std::string::npos) {
            std::ostringstream convert;
            convert << futureWindowSize << "/" << cacheSize[i] ;
            return std::string(policyName[i] +  "-" + (convert.str()));
        }
        else
            return policyName[i];
    }

    inline std::string PrintTestName() {
        return std::string(testName);
    }

    inline std::string GetTraceName() {
        return std::string(traceName);
    }


private:
    void allocateArrays(int totalLevels);
    uint64_t myString2intConverter(std::string temp);
};



#endif
