#include <unordered_map>
#include <queue>
#include "global.h"
#include "min.h"
#include "configuration.h"
#include "parser.h"

using namespace std;

void AccessOrdering::build(){
	ifstream zahraStream;

	zahraStream.open(_gConfiguration.traceName);

	if( ! zahraStream.good() ){
		cerr<<"can not open file to build access ordering"<<endl;
		ExitNow(1);
	}
	PRINTV(cerr<<"Building AccessOrdering...."<<endl;);
	reqAtom newAtom;
	uint32_t lineNo =0 ;
	while(getAndParseMSR(zahraStream ,&newAtom)){
		uint64_t key = newAtom.fsblkno;
		lineNo = newAtom.lineNo;
		assert(lineNo);
		AOHashTable::iterator it;
		it = hashTable.find(key);
		if( it == hashTable.end() ){
			// key is not availble in the hashTable
			queue<uint32_t> tempQ;
			tempQ.push(lineNo);
			pair<uint64_t,queue<uint32_t>> temP(key,tempQ);
			pair<AOHashTable::iterator,bool> ret;
			ret = hashTable.insert(temP);
			assert( ret.second == true ); //suppose to insert successfully
		}
		else{
			// key is availble
			assert( it->first == key );
			queue<uint32_t> *tempQPoint; 
			tempQPoint = &(it->second);
			assert(tempQPoint->size() != 0);
			assert(tempQPoint->back() < lineNo );
			assert(tempQPoint->front() < lineNo );
			tempQPoint->push(lineNo);
		}
		newAtom.clear();
	}
	PRINTV(cerr<<"Building AccessOrdering finished. "<<endl;);
	PRINTV(cerr<<" ... unique requests: "<< hashTable.size()<<endl; );
	PRINTV(cerr<<" ... total requests: "<< lineNo<< endl; );
	zahraStream.close();
}

uint32_t AccessOrdering::nextAccess(uint64_t key, uint32_t currLine)
{
	assert(key);
	AOHashTable::iterator it; 
	it = hashTable.find(key);
	
	if( it == hashTable.end())
		return 0; 
	else{
		assert( it->first == key);
		queue<uint32_t> tempQ;
		tempQ = it->second;
		while( tempQ.front() < currLine ){
			tempQ.pop(); // skip
			if(tempQ.size() == 0 ){
				hashTable.erase(key);
				return INF;
			}
		}
		return tempQ.front();
	}
}
