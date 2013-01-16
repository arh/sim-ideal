
#ifndef SHAREDDS_H
#define SHAREDDS_H

#include <unordered_map>
#include <unordered_set>
#include <map>
#include <queue>
#include <list>
#include <set>
#include "assert.h"
#include "iostream"
#include "global.h"
#include "baseCache.h"



using namespace std;
extern 	deque<reqAtom> memTrace; // in memory trace file
//************** AccessOrdering **************//



class AccessOrdering{
private:
	typedef unordered_map<uint64_t,queue<uint32_t>> AOHashTable; //access ordering hash table
	AOHashTable hashTable;
	bool builded;
	
public:
	AccessOrdering(){
		hashTable.clear();
		builded=false;
	}
	void pageBaseBuild();
	void blockBaseBuild();
	
	uint32_t nextAccess( uint64_t key, uint32_t currLine);
	
	
};

// lineno_2_key heap nodes
class HeapAtom{
public:
	uint32_t lineNo;
	uint64_t key;
	HeapAtom(){
		lineNo=0;
		key=0;
	}
	HeapAtom(uint32_t newLine, uint64_t newKey){
		lineNo =  newLine; 
		key  = newKey;
	}
};

class CompHeapAtom{
public:
	bool operator()( const HeapAtom & a , const HeapAtom & b ){
		return (a.lineNo < b.lineNo);
	}
};





// 	uniqFutureDist();
uint32_t getFutureBlkDist(const uint64_t SsdBlknokey, uint32_t currLine);


#endif