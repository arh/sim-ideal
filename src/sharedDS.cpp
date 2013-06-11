#include <unordered_map>
#include <queue>
#include <deque>
#include "global.h"
#include "min.h"
#include "configuration.h"
#include "parser.h"



using namespace std;
//************** AccessOrdering **************//

void AccessOrdering::pageBaseBuild()
{
    assert(builded == false);
    PRINTV(logfile << "Building AccessOrdering...." << endl;);
    uint32_t lineNo = 0;
    deque<reqAtom>::iterator it = memTrace.begin(); // iterate over the memTrace

    for( ; it != memTrace.end() ; ++ it ) {
        uint64_t key = it->fsblkno;
        lineNo = it->lineNo;
        assert(lineNo);
        AOHashTable::iterator it;
        it = hashTable.find(key);

        if( it == hashTable.end() ) {
            // key is not availble in the hashTable
            queue<uint32_t> tempQ;
            tempQ.push(lineNo);
            assert(tempQ.size() == 1 );
            pair<uint64_t, queue<uint32_t>> temP(key, tempQ);
            pair<AOHashTable::iterator, bool> ret;
            ret = hashTable.insert(temP);
            assert( ret.second == true ); //suppose to insert successfully
        }
        else {
            // key is availble
            assert( it->first == key );
            queue<uint32_t> *tempQPoint;
            tempQPoint = &(it->second);
            assert(tempQPoint->size() != 0);
            assert(tempQPoint->back() < lineNo );
            assert(tempQPoint->front() < lineNo );
            tempQPoint->push(lineNo);
        }
    }

    PRINTV(logfile << "Building Page Based AccessOrdering finished. " << endl;);
    PRINTV(logfile << " ... unique page access: " << hashTable.size() << endl; );
    PRINTV(logfile << " ... total page access: " << lineNo << endl; );
    builded = true;
}


void AccessOrdering::blockBaseBuild()
{
    assert(builded == false);
    PRINTV(logfile << "Building AccessOrdering...." << endl;);
    uint32_t lineNo = 0;
    deque<reqAtom>::iterator it = memTrace.begin(); // iterate over the memTrace

    // 	pair<deque<reqAtom>::iterator,bool> ret;

    for( ; it != memTrace.end() ; ++ it ) {
        uint64_t key = it->ssdblkno;
        lineNo = it->lineNo;
        assert(lineNo);
        AOHashTable::iterator it;
        it = hashTable.find(key);

        if( it == hashTable.end() ) {
            // key is not availble in the hashTable
            queue<uint32_t> tempQ;
            tempQ.push(lineNo);
            assert(tempQ.size() == 1 );
            pair<uint64_t, queue<uint32_t>> temP(key, tempQ);
            pair<AOHashTable::iterator, bool> ret;
            ret = hashTable.insert(temP);
            assert( ret.second == true ); //suppose insert successfully
        }
        else {
            // key is availble
            assert( it->first == key );
            queue<uint32_t> *tempQPoint;
            tempQPoint = &(it->second);
            assert(tempQPoint->size() != 0);
            assert(tempQPoint->back() <= lineNo );
            assert(tempQPoint->front() <= lineNo );
            tempQPoint->push(lineNo);
        }
    }

    PRINTV(logfile << "Building Block Based AccessOrdering finished. " << endl;);
    PRINTV(logfile << " ... unique Block access: " << hashTable.size() << endl; );
    PRINTV(logfile << " ... total Block access: " << lineNo << endl; );
    builded = true;
}



uint32_t AccessOrdering::nextAccess(uint64_t key, uint32_t currLine)
{
    if(builded == false) {
        cerr << "Access ordering list is not ready" << endl;
        assert(0);
    }

    AOHashTable::iterator it;
    it = hashTable.find(key);

    if( it == hashTable.end()) {
        cerr << "Block " << key << " is not availble in Access ordering list" << endl;
        assert(0);
        return 0; // remove compiler warning
    }
    else {
        assert( it->first == key);
        queue<uint32_t> tempQ;
        tempQ = it->second;

        while( tempQ.front() <= currLine ) {
            tempQ.pop(); // skip

            if(tempQ.size() == 0 ) {
                hashTable.erase(key);
                return INF;
            }
        }

        return tempQ.front();
    }
}
