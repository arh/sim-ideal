#include "configuration.h"
#include "boost/lexical_cast.hpp"
#include "boost/program_options.hpp"

void Configuration::allocateArrays(int totalLevels)
{
    outTraceStream = new std::ofstream[totalLevels];
    policyName = new std::string[totalLevels];
    cacheSize =  new uint64_t[totalLevels];
    cachePageSize = new uint64_t[totalLevels];
    cacheBlkSize = new uint64_t[totalLevels];
    ssd2fsblkRatio = new uint32_t[totalLevels];

    for(int i = 0 ; i < totalLevels ; i++) {
        cacheSize[i] = cachePageSize[i] = cacheBlkSize[i] = 0;
    }
}

uint64_t Configuration::myString2intConverter(std::string str)
{
    uint64_t tempInt = 0;

    try {
        tempInt = boost::lexical_cast<uint64_t>(str) ;
    }
    catch(...) {
        // for some (unkown) reason, lexical_cast cannot convert K M G !
        if(str.find("K") != std::string::npos) {
            size_t pos = str.find("K") ;
            std::istringstream(str.substr(0, pos)) >> tempInt ;
            tempInt *= 1024 ; //convert to K
        }

        if(str.find("M") != std::string::npos) {
            size_t pos = str.find("K") ;
            std::istringstream(str.substr(0, pos)) >> tempInt ;
            tempInt *= 1024 * 1024 ; //convert to M
        }

        if(str.find("G") != std::string::npos) {
            size_t pos = str.find("K") ;
            std::istringstream(str.substr(0, pos)) >> tempInt ;
            tempInt *= 1024 * 1024 * 1024 ; //convert to G
        }
    }

    return  tempInt;
}


bool Configuration::read(int argc, char **argv)
{
    if(argc < 4)
        return false;

    //read trace file name
    traceName = argv[1];

    if(GetTraceName().compare("stdin") != 0) {
        _gTraceBased = true;
    }
    else {
        std::cerr << " Error: read from stdin is not implemented" << std::endl;
        exit(1);
    }

    boost::property_tree::ptree pTree;
    logStream << "Parsing configuration file: " << argv[2] << std::endl;

    try {
        read_info(argv[2], pTree);
    }
    catch(boost::property_tree::info_parser_error e) {
        throw std::runtime_error(std::string(e.what())) ;
    }

    try {
        std::string cacheStr[] = { "L1_Cache", "L2_Cache", "L3_Cache", "L4_Cache"};
        totalLevels = pTree.get<int>("Global.levels");

        if(totalLevels == 0 || totalLevels > 4) {
            std::cerr << "Cfg file error, Invalid number of cache levels " << std::endl;
            exit(1);
        }

        std::string tempStr;
        tempStr = pTree.get<std::string>("Global.fsBlkSize");
        fsblkSize = myString2intConverter(tempStr);
        allocateArrays(totalLevels);

        try {
            tempStr = pTree.get<std::string>("Global.logFile");
        }
        catch(...) {
            //no log file specified
            tempStr.clear();
        }

        if(! tempStr.empty()) {
            logStream.open(tempStr.c_str(), std::ios::trunc);
            //print start time
            time_t now = time(0);
            tm *localtm = localtime(&now);
            logStream << "Start Logging at " << asctime(localtm) << std::endl;
        }
               

        try {
            tempStr = pTree.get<std::string>("Global.writeOnly");
        }
        catch(...) {
            //no log file specified
            tempStr.clear();
        }

        if(! tempStr.empty()) {
            writeOnly = (bool) myString2intConverter(tempStr);
        }

        ///ziqi: read out the value for seqThreshold
        try {
            tempStr = pTree.get<std::string>("Global.seqThreshold");
        }
        catch(...) {
            //no log file specified
            tempStr.clear();
        }

        if(! tempStr.empty()) {
	    seqThresholdStringType = tempStr;
            seqThreshold = (int) myString2intConverter(tempStr);
        }

        for(int i = 0; i < totalLevels ; ++i) {
            tempStr = pTree.get<std::string>	(std::string(cacheStr[i] + "." + "size"));
            cacheSize[i] = myString2intConverter(tempStr);
            tempStr = pTree.get<std::string>	(std::string(cacheStr[i] + "." + "pageSize"));
            cachePageSize[i] = myString2intConverter(tempStr);
            tempStr = pTree.get<std::string>	(std::string(cacheStr[i] + "." + "blkSize"));
            cacheBlkSize[i] = myString2intConverter(tempStr);
            ssd2fsblkRatio[i] = cacheBlkSize[i] / fsblkSize;

            try {
                tempStr = pTree.get<std::string>(std::string(cacheStr[i] + "." + "outTraceFile"));
            }
            catch(...) {
                //no log file specified
                tempStr.clear();
            }

            if(! tempStr.empty()) {
                outTraceStream[i].open(tempStr.c_str(), std::ios::trunc);

                if(i == totalLevels - 1) {
                    cache2diskPipeFileName = tempStr;
                }
            }

            policyName[i] = pTree.get<std::string>	(std::string(cacheStr[i] + "." + "policy"));
	    
	    ///ziqi: read out diskSimInputTrace name from cfg file
	    try {
		tempStr = pTree.get<std::string>("Global.diskSimInputTrace");
	    }
	    catch(...) {
		//no log file specified
		tempStr.clear();
	    }

	    if(! tempStr.empty()) {
	        std::string traceNameStringType = traceName;
		std::cout<<"traceNameStringType "<<traceNameStringType<<std::endl;
	        int lastSlash = traceNameStringType.find_last_of('/');
		int lastDot = traceNameStringType.find_last_of('.');
		std::cout<<"lastSlash "<<lastSlash<<" lastDot "<<lastDot<<std::endl;
		diskSimInputTraceName = tempStr+"-Policy_"+policyName[i]+"-Threshold_"+seqThresholdStringType+"-MSRTrace_"+traceNameStringType.substr(lastSlash+1,lastDot-lastSlash-1)+".trace";
		diskSimInputStream.open((diskSimInputTraceName).c_str(), std::ios::trunc);
	    }
	    ///end here
	    
            //read policy dependent configs
            if(policyName[i].find("owbp") != std::string::npos) {
                tempStr = pTree.get<std::string> (std::string(cacheStr[i] + "." + "policy" + "." + "futureWindowSize"));
                futureWindowSize = myString2intConverter(tempStr) ;
            }
        }

        // read disk simulator configs
        tempStr.clear();

        try {
            tempStr = pTree.get<std::string>(std::string("Disk_Array.diskSimulator"));
        }
        catch(...) {
            // no disk Simulator provided
            tempStr.clear();
        }

        if(! tempStr.empty()) {
            diskSimuExe = tempStr;
            diskSimPath = pTree.get<std::string>(std::string("Disk_Array.simulatorPath"));
            diskSimParv = pTree.get<std::string>(std::string("Disk_Array.parvFile"));
            diskSimOutv = pTree.get<std::string>(std::string("Disk_Array.outvFile"));

            if(cache2diskPipeFileName.empty()) {
                std::cerr << "Error: There is no output Trace file specified in the cfg for the last level cache to feed the Disk Simulator";
                exit(1);
            }
        }
        
        tempStr.clear();
	
	try {
            tempStr = pTree.get<std::string>(std::string("Seq_Length.seqLengthAnalysisApp"));
        }
        catch(...) {
            // no disk Simulator provided
            tempStr.clear();
        }

        if(! tempStr.empty()) {
            analysisAppExe = tempStr;
            analysisAppPath = pTree.get<std::string>(std::string("Seq_Length.analysisAppPath"));
        }
    }
    catch(boost::property_tree::ptree_bad_path e) {
        throw std::runtime_error(std::string(e.what())) ;
    }

    testName	= argv[3];
    IFHIST(birdHist = new uint64_t[futureWindowSize];);
    IFHIST(pirdHist = new uint64_t[futureWindowSize];);
    IFHIST(initHist(););

    //TODO generalize this with boost program_options
    if(argc > 4)
        if(strcmp(argv[4], "-s") == 0) {
            cacheSize[0] = myString2intConverter(std::string(argv[5]));
        }

    return true;
}

void Configuration::initHist()
{
    assert(futureWindowSize);

    for(unsigned i = 0 ; i < futureWindowSize ; ++i) {
        birdHist[i] = 0;
        pirdHist[i] = 0;
    }
}

Configuration::Configuration()
{
    traceName = NULL;
    policyName = NULL;
    cacheSize = NULL;
    outTraceStream = NULL;
    totalLevels = 0;
    writeOnly = false;
    ///ziqi
    seqThreshold = 0;
    testName = 0;
    // 		ssdblkSize = 16384 ;
    maxLineNo = 0;
    birdHist = NULL;
    pirdHist = NULL;
}


Configuration::~Configuration()
{
    IFHIST(delete pirdHist;);
    IFHIST(delete birdHist;);
    traceStream.close();
    //print end time
    time_t now = time(0);
    tm *localtm = localtime(&now);
    logStream << "End Logging at " << asctime(localtm) << std::endl;
    logStream.close();

    for(int i = 0 ; i < totalLevels ; ++i)
        outTraceStream[i].close();

    delete [] cacheSize;
    delete [] policyName;
    delete [] cacheBlkSize;
    delete [] cachePageSize;
    delete [] ssd2fsblkRatio;
    delete [] outTraceStream;
}

