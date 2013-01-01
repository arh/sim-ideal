#include <iostream>
#include <time.h>
#include "global.h"
#include "cpp_framework.h"
#include "configuration.h"
#include "parser.h"
#include "lru_stl.h"
#include <queue>

using namespace std;

////////////////////////////////////////////////////////////////////////////////
//GLOBALS
////////////////////////////////////////////////////////////////////////////////

extern Configuration	_gConfiguration;




void	readTrace()
{
	_gConfiguration.traceStream.open(_gConfiguration.traceName, ifstream::in);

	if(! _gConfiguration.traceStream.good()) {
		PRINT(cout << " Error: Can not open trace file : " << _gConfiguration.traceName << endl;);
		exitNow(1);
	}
}

void	Initialize(int argc, char **argv, queue<reqAtom> & memTrace)
{
	if(!_gConfiguration.read(argc, argv)) {
		cerr << "USAGE: <tracefilename> " << endl;
		exit(-1);
	}

	readTrace();
	cerr << "Configuration and setup done" << endl;
	srand(0);
}

int main(int argc, char **argv)
{
	lru_stl< uint64_t , cacheAtom> LRUCache(cacheAll, _gConfiguration.L1cacheSize);
	queue<reqAtom> memTrace; // in memory trace file
	//read benchmark configuration
	Initialize(argc, argv,memTrace);
// 	RunBenchmark();
// 	ExitNow(0);
	return 0;
}
