#ifndef GLOBAL_H
#define GLOBAL_H
#include "configuration.h"

#ifdef DEBUG
#define PRINT(X)	do { X	}while(false)
#else
#define PRINT(X)
#endif



#ifdef VERB
#define PRINTV(X)	do { X	}while(false)
#else
#define PRINTV(X)
#endif

extern Configuration	_gConfiguration;
const uint32_t READ = 1;
const uint32_t WRITE = 2;
const uint32_t MISS = 4;
const uint32_t HIT = 8;
const uint32_t EVICT = 16;


class reqAtom
{
public:
	uint64_t fsblkno; //file system block number (from trace file)
	uint32_t reqSize; // request size from trace file
	uint64_t ssdblkno; //ssd block number
	double issueTime; // time stapt in the trace
	uint32_t lineNo; // line number in the trace
	uint32_t flags;
	reqAtom() {
		clear();
	}
	reqAtom(uint32_t tLineno , double time, uint32_t tblkno , uint32_t treqSize  , uint32_t rw) {
		lineNo = tLineno;
		issueTime = time ;
		fsblkno = tblkno;
		reqSize = treqSize;
		flags = rw;
		ssdblkno = fsblkno / _gConfiguration.ssd2fsblkRatio;
	}
	void clear(){
		fsblkno = 0; //file system block number
		ssdblkno = 0; //ssd block number
		issueTime = 0; // time stapt in the trace
		lineNo = 0 ; // line number in the trace
		flags = 0;
		reqSize = 0;
	}

};

class cacheAtom
{
private:
	reqAtom req;
public:
	cacheAtom(reqAtom newn) {
		req = newn;
	}
	cacheAtom(){
		
	}
	void clear(){
		req.clear();
	}
};

void exitNow(unsigned code);


cacheAtom cacheAll(const unsigned long long int& key, cacheAtom new_value);

#endif
