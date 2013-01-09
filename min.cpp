#include <unordered_map>
#include <queue>
#include "global.h"
#include "min.h"
#include "configuration.h"
#include "parser.h"

using namespace std;

//************** AccessOrdering **************//

void AccessOrdering::pageBaseBuild(){
	
	ifstream zahraStream;

	zahraStream.open(_gConfiguration.traceName);

	if( ! zahraStream.good() ){
		cerr<<"can not open file to build access ordering"<<endl;
		ExitNow(1);
	}
	PRINTV(logfile<<"Building AccessOrdering...."<<endl;);
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
	PRINTV(logfile<<"Building Page Based AccessOrdering finished. "<<endl;);
	PRINTV(logfile<<" ... unique page access: "<< hashTable.size()<<endl; );
	PRINTV(logfile<<" ... total page access: "<< lineNo<< endl; );
	zahraStream.close();
	builded = true; 
}


void AccessOrdering::blockBaseBuild(){
	
	assert(builded==false);
	ifstream zahraStream;
	
	zahraStream.open(_gConfiguration.traceName);
	
	if( ! zahraStream.good() ){
		cerr<<"can not open file to build access ordering"<<endl;
		ExitNow(1);
	}
	PRINTV(logfile<<"Building AccessOrdering...."<<endl;);
	reqAtom newAtom;
	uint32_t lineNo =0 ;
	while(getAndParseMSR(zahraStream ,&newAtom)){
		uint64_t key = newAtom.ssdblkno;
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
	PRINTV(logfile<<"Building Block Based AccessOrdering finished. "<<endl;);
	PRINTV(logfile<<" ... unique Block access: "<< hashTable.size()<<endl; );
	PRINTV(logfile<<" ... total Block access: "<< lineNo<< endl; );
	zahraStream.close();
	builded = true; 
}



uint32_t AccessOrdering::nextAccess(uint64_t key, uint32_t currLine)
{
	if(builded== false){
		cerr<<"Access ordering list is not ready"<<endl;
		assert(0);
	}
	
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
//**************   **************//