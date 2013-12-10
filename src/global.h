#ifndef GLOBAL_H
#define GLOBAL_H
#include <vector>
#include "configuration.h"


#ifdef DEBUG
#define PRINT(X)	do { X	}while(false)
#else
#define PRINT(X)
#endif

#ifdef DEBUG
#define IFDEBUG(X)	do { X	}while(false)
#else
#define IFDEBUG(X)
#endif


#ifdef VERB
#define PRINTV(X)	do { X	}while(false)
#else
#define PRINTV(X)
#endif

#ifdef HIST
#define IFHIST(X)	do { X	}while(false)
#else
#define IFHIST(X)
#endif


extern Configuration	_gConfiguration;

#define logfile _gConfiguration.logStream

///ziqi: defined in configuration.h
#define DISKSIMINPUTSTREAM _gConfiguration.diskSimInputStream

const uint32_t READ = 1;
const uint32_t WRITE = 2;
const uint32_t PAGEMISS = 4;
const uint32_t PAGEHIT = 8;
const uint32_t EVICT = 16;
const uint32_t BLKMISS = 32;
const uint32_t BLKHIT = 64;
const uint32_t COLD2COLD = 128;
const uint32_t COLD2HOT = 256;
//ziqi: denote dirty page
const uint32_t DIRTY = 512;
const uint32_t SEQEVICT = 1024;
const uint32_t LESSSEQEVICT = 2048;

//Bypassing state
const uint32_t BYPASS = 4196<<0; //by pass 0th level
// const uint32_t BYPASS = 4196<<1; //reserved for bypass 1th level
// const uint32_t BYPASS = 4196<<2; //reserved for bypass 2nd level
// const uint32_t BYPASS = 4196<<3; //reserved for bypass 3rd level

//new flag should start from 4196*16 
const uint32_t INF = 0xFFFFFFFF;

/* Objects of this type hold the basic infomation about a single request */
class reqAtom
{
public:
	uint64_t fsblkno; //file system block number (from trace file)
	uint32_t reqSize; // request size from trace file
	uint64_t ssdblkno; //ssd block number
	///ziqi: change back to double from unsigned on Jun 19 2013
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
		//TODO: fix this line to dynamically adapt with multi levels
		ssdblkno = fsblkno / _gConfiguration.ssd2fsblkRatio[0];
	}
	void clear() {
		fsblkno = 0; //file system block number
		ssdblkno = 0; //ssd block number
		issueTime = 0; // time stapt in the trace
		lineNo = 0 ; // line number in the trace
		flags = 0;
		reqSize = 0;
	}

};

/* This is a wrapper class for requests that are stored in the cache metadata type */
class cacheAtom
{
private:
	reqAtom req;
public:
	cacheAtom(reqAtom newn) {
		req = newn;
	}
	cacheAtom() {
	}
	void clear() {
		req.clear();
	}
	uint32_t getLineNo() const {
		return req.lineNo;
	}
	uint64_t getSsdblkno() const {
		return req.ssdblkno;
	}
	uint64_t getFsblkno() const {
		return req.fsblkno;
	}
	reqAtom getReq() const {
		return req;
	}
	void update(const cacheAtom &newValue) {
		req = newValue.getReq();
	}

	///ziqi
	uint32_t getFlags() const {
		return req.flags;
	}
	///ziqi
	void updateFlags(uint32_t outerFlags) {
		req.flags = outerFlags;
	}
	void addFlags( uint32_t newFlags){
		req.flags = req.flags | newFlags ;
	}
};

class reqPacket {
private:
	std::vector<reqAtom> reqList;
	uint32_t packetFlags;
public:
	reqPacket( reqAtom newAtom) {
		reqList.push_back(newAtom);
		packetFlags = 0;
	}
	reqPacket(){
		packetFlags = 0;
	}
	uint32_t getSize(){
		return reqList.size();
	}
	std::vector<reqAtom> getReqList(){
		return reqList;
	}
	void append( reqPacket newPacket ){
		std::vector<reqAtom> newReqList = newPacket.getReqList();
		if( newReqList.size() )
			reqList.insert(reqList.end(), newReqList.begin() ,newReqList.end() );
	}
	void append( const cacheAtom newCacheAtom){
		reqAtom newReq = newCacheAtom.getReq();
		assert(newReq.fsblkno);
		reqList.push_back(newReq);
	}
	reqAtom operator[] (int i){
		return reqList[i];
	}
};

void ExitNow(unsigned code);


cacheAtom cacheAll(const uint64_t &key, cacheAtom new_value);

#endif
