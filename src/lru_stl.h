//
// C++ Interface: lru_stl
//
// Description:
//
//
// Author: ARH,,, <arh@aspire-one>, (C) 2011
//
// Copyright: See COPYING file that comes with this distribution
//

#ifndef LRU_STL_H
#define LRU_STL_H

#include <map>
#include <list>
#include "cassert"
#include "iostream"
#include "iomanip"
#include "bitset"
#include "global.h"
#include "baseCache.h"

using namespace std;

///ziqi: in access(), if the value's time stamp is the first one that equal or bigger than a multiple of 30s, then flushing back all the dirty pages in buffer cache.
///Then change the dirty pages status to clean. Log these dirty pages into DiskSim input trace.
///ziqi: in access(), for status is write, add dirty page notation to the status.
///(done) ziqi: in remove(), modify it as lru_ziqi.h. Log the evicted dirty page. 


// Class providing fixed-size (by number of records)
// LRU-replacement cache of a function with signature
// V f(K)
template <typename K, typename V>
class PageLRUCache : public TestCache<K, V>
{
public:
// Key access history, most recent at back
    typedef list<K> key_tracker_type;
// Key to value and key history iterator
    typedef map
    < K, pair<V, typename key_tracker_type::iterator> > 	key_to_value_type;
// Constuctor specifies the cached function and
// the maximum number of records to be stored.
    PageLRUCache(
        V(*f)(const K & , V),
        size_t c,
        unsigned levelMinus
    ) : _fn(f) , _capacity(c), levelMinusMinus(levelMinus)  {
        ///ARH: Commented for single level cache implementation
//         assert ( _capacity!=0 );
    }
    // Obtain value of the cached function for k

    uint32_t access(const K &k  , V &value, uint32_t status) {
        assert(_capacity != 0);
        PRINTV(logfile << "Access key: " << k << endl;);	
	PRINTV(logfile << "At time: " << value.getReq().issueTime << endl;);
	PRINTV(logfile << "Key dirty bit status: " << bitset<10>(status)<< endl;);
	
	typename key_tracker_type::iterator itTracker;
	typename key_to_value_type::iterator itDirty;
	typename key_to_value_type::iterator itSeqTemp;
	
///ziqi: on Aug 9, 2013: denote the first sequential fs block number that counting consecutive pages after the victim page
        uint64_t firstSeqFsblknoForAfter = 0;
///ziqi: on Aug 9, 2013: denote the first sequential fs block number that counting consecutive pages before the victim page
        uint64_t firstSeqFsblknoForBefore = 0;
	
	int seqLength = 0;
	
	///ziqi: time gap of every two flush backs in milliseconds
	uint32_t flushTimeGap = 30000;
	///ziqi: denote how many 30s flush back has been operated
	static uint32_t multipleFlushTimeGap = 1;
	
	uint32_t CLEAN = ~DIRTY;
	
	///ziqi: if the accessed entry's issueTime is the first one bigger or equal than a multiple of 30s, prepare to flush back all the dirty pages residing on cache
	if(value.getReq().issueTime >= (flushTimeGap*multipleFlushTimeGap)){
	  
	  ///ziqi: loop through cache and find out those dirty pages. Group sequential ones together and log to DiskSim input trace.
	  ///ziqi: CLEAN is used to toggle DIRTY status after the dirty page has been flushed back.
	  for(itTracker = _key_tracker.begin(); itTracker != _key_tracker.end(); itTracker++) {
	    itDirty = _key_to_value.find(*itTracker);
	    firstSeqFsblknoForAfter = *itTracker;
	    firstSeqFsblknoForBefore = *itTracker;
	    seqLength = 0;
	    
	    if(itDirty->second.first.getReq().flags & DIRTY) {   
///ziqi: on Aug 9, 2013: find the number of consecutive blocks after the selected page   	      
	      while(true) {
		itSeqTemp = _key_to_value.find(firstSeqFsblknoForAfter);
		
		if(itSeqTemp == _key_to_value.end() || !((itSeqTemp->second.first.getReq().flags) & DIRTY)) {
                  break;
                }
                else {
	          itSeqTemp->second.first.updateFlags(itSeqTemp->second.first.getReq().flags & CLEAN);	  
                  firstSeqFsblknoForAfter++;
                  seqLength++;
		}
              }
              
///ziqi: on Aug 9, 2013: find the number of consecutive blocks before the selected page               
              firstSeqFsblknoForBefore--;
              while(true) {
		itSeqTemp = _key_to_value.find(firstSeqFsblknoForBefore);
		
		if(itSeqTemp == _key_to_value.end() || !((itSeqTemp->second.first.getReq().flags) & DIRTY)) {
                  ///ziqi: DiskSim format Request_arrival_time Device_number Block_number Request_size Request_flags
                  ///ziqi: Device_number is set to 1. About Request_flags, 0 is for write and 1 is for read
///ziqi: on Aug 9, 2013: made change to Block_number, because the starting block number is changed.
	          PRINTV(DISKSIMINPUTSTREAM << setfill(' ')<<left<<fixed<<setw(25)<<flushTimeGap*multipleFlushTimeGap<<left<<setw(8)<<"0"<<left<<fixed<<setw(12)<<(firstSeqFsblknoForBefore+1)<<left<<fixed<<setw(8)<<seqLength<<"0"<<endl;);	
                  break;
                }
                else {
	          itSeqTemp->second.first.updateFlags(itSeqTemp->second.first.getReq().flags & CLEAN);
                  firstSeqFsblknoForBefore--;
                  seqLength++;
		}
              }
             
	      itDirty->second.first.updateFlags(itDirty->second.first.getReq().flags & CLEAN);
	    }
	  }
	  
          multipleFlushTimeGap += (uint32_t(value.getReq().issueTime) - flushTimeGap*multipleFlushTimeGap) / flushTimeGap + 1;
	  PRINTV(logfile << "multipleFlushTimeGap: " << multipleFlushTimeGap << endl;);
	}
	
	///ziqi: if request is write, mark the page status as DIRTY
        if(status & WRITE) {
            status |= DIRTY;
            value.updateFlags(status);
        }
        
        
	
// Attempt to find existing record
        const typename key_to_value_type::iterator it	= _key_to_value.find(k);

        if(it == _key_to_value.end()) {
// We don’t have it:
            PRINTV(logfile << "Miss on key: " << k << endl;);
// Evaluate function and create new record
            const V v = _fn(k, value);
            status |=  insert(k, v);
            PRINTV(logfile << "Insert done on key: " << k << endl;);
	    PRINTV(logfile << "Key bit status: " << bitset<10>(value.getReq().flags) << endl;);
	    PRINTV(logfile << "Cache utilization: " << _key_to_value.size() <<"/"<<_capacity <<endl<<endl;);
            return (status | PAGEMISS);
        }
        else {
            PRINTV(logfile << "Hit on key: " << k << endl;);
	    
            /*
            // We do have it. Before returning value,
            // update access record by moving accessed
            // key to back of list.
            _key_tracker.splice(
                    _key_tracker.end(),
                    _key_tracker,
                    (*it).second.second
            );
            (*it).second.second = _key_tracker.rbegin().base();
            return (status | PAGEHIT | BLKHIT);

            */
	    ///ziqi: find out the original page's dirty bit status by "it->second.first.getReq().flags & DIRTY", and add it to new value by "| value.getReq().flags"
	    value.updateFlags((it->second.first.getReq().flags & DIRTY) | value.getReq().flags);
            _key_to_value.erase(it);
            _key_tracker.remove(k);
            assert(_key_to_value.size() < _capacity);
            const V v = _fn(k, value);
            // Record k as most-recently-used key
            typename key_tracker_type::iterator itNew
            = _key_tracker.insert(_key_tracker.end(), k);
            // Create the key-value entry,
            // linked to the usage record.
            _key_to_value.insert(make_pair(k, make_pair(v, itNew)));
	    
	    PRINTV(logfile << "Key bit status: " << bitset<10>(value.getReq().flags) << endl<<endl;);
	    
            return (status | PAGEHIT | BLKHIT);
        }
    } //end operator access


    unsigned long long int get_min_key() {
        return (_key_to_value.begin())->first;
    }

    unsigned long long int get_max_key() {
// 			std::map< K, std::pair<V,typename key_tracker_type::iterator> >::iterator it;
        return (_key_to_value.rbegin())->first;
    }
    
    
    ///ziqi: alireza version
    /*
    void remove(const K &k) {
        PRINTV(logfile << "Removing key " << k << endl;);
// Assert method is never called when cache is empty
        assert(!_key_tracker.empty());
// Identify  key
        const typename key_to_value_type::iterator it
        = _key_to_value.find(k);
        assert(it != _key_to_value.end());
        PRINTV(logfile << "Remove value " << endl;);
// Erase both elements to completely purge record
        _key_to_value.erase(it);
        _key_tracker.remove(k);
    }
    */
    
///ziqi: k is used to denote the actual entry with key value of "k" to be evicted
///ziqi: v is used to denote the original entry that passed to access() method. We only replace the time stamp of k by the time stamp of v
    void remove(const K &k, const V &v) {
        PRINTV(logfile << "Removing key " << k << endl;);
// Assert method is never called when cache is empty
        assert(!_key_tracker.empty());
// Identify  key
        typename key_to_value_type::iterator it = _key_to_value.find(k);
	assert(it != _key_to_value.end());
	
	///PRINTV(logfile << "Before eviting, key bit status: " << bitset<10>(it->second.first.getReq().flags) << endl;);
	
	if(it->second.first.getReq().flags & DIRTY) {
	
///ziqi: DiskSim format Request_arrival_time Device_number Block_number Request_size Request_flags
///ziqi: Device_number is set to 1. About Request_flags, 0 is for write and 1 is for read
	  PRINTV(DISKSIMINPUTSTREAM << setfill(' ')<<left<<fixed<<setw(25)<<v.getReq().issueTime<<left<<setw(8)<<"0"<<left<<fixed<<setw(12)<<it->second.first.getReq().fsblkno<<left<<fixed<<setw(8)<<it->second.first.getReq().reqSize<<"0"<<endl;);	
	  
	  PRINTV(logfile << "Remove value " << endl;);
	  
	  // Erase both elements to completely purge record	
	  PRINTV(logfile << "evicting dirty key " << k <<  endl;);
	  PRINTV(logfile << "Key dirty bit status: " << bitset<10>(it->second.first.getReq().flags) << endl;);
	  it = _key_to_value.find(k);
	  assert(it != _key_to_value.end());
	  _key_to_value.erase(it);
	  _key_tracker.remove(k);
	  
	  PRINTV(logfile << "Cache utilization: " << _key_to_value.size() <<"/"<<_capacity <<endl<<endl;);
	}
	else {
	  PRINTV(logfile << "evicting clean key without flushing back to DiskSim input trace " << k <<  endl;);
	  ///PRINTV(logfile << "Key clean bit status: " << bitset<10>(it->second.first.getReq().flags) << endl;);
	  it = _key_to_value.find(k);
	  assert(it != _key_to_value.end());
	  _key_to_value.erase(it);
	  _key_tracker.remove(k);  
	  PRINTV(logfile << "Cache utilization: " << _key_to_value.size() <<"/"<<_capacity <<endl<<endl;);
	}
	
    }
    
private:

// Record a fresh key-value pair in the cache
    int insert(const K &k, const V &v) {
        PRINTV(logfile << "insert key " << k  << endl;);
	PRINTV(logfile << "Key bit status: " << bitset<10>(v.getReq().flags) << endl;);
        int status = 0;
// Method is only called on cache misses
        assert(_key_to_value.find(k) == _key_to_value.end());

// Make space if necessary
        if(_key_to_value.size() == _capacity) {
            PRINTV(logfile << "Cache is Full " << _key_to_value.size() << " sectors" << endl;);
            evict(v);
            status = EVICT;
        }
        
// Record k as most-recently-used key
        typename key_tracker_type::iterator it
        = _key_tracker.insert(_key_tracker.end(), k);
// Create the key-value entry,
// linked to the usage record.
        _key_to_value.insert(make_pair(k, make_pair(v, it)));
// No need to check return,
// given previous assert.
// 			add_sram_entry(k,false);
        return status;
    }
    ///ziqi: alireza version
    /*
// Purge the least-recently-used element in the cache
    void evict() {
// Assert method is never called when cache is empty
        assert(!_key_tracker.empty());
// Identify least recently used key
        const typename key_to_value_type::iterator it
        = _key_to_value.find(_key_tracker.front());
        assert(it != _key_to_value.end());
        PRINTV(logfile << "evicting victim key " << (*it).first <<  endl;);
// Erase both elements to completely purge record
        _key_to_value.erase(it);
        _key_tracker.pop_front();
    }
    */
    
    // Purge the least-recently-used element in the cache
    void evict(const V &v) {
// Assert method is never called when cache is empty
        assert(!_key_tracker.empty());
// Identify least recently used key
	
	typename key_tracker_type::iterator itTracker = _key_tracker.begin();

        assert(itTracker != _key_tracker.end());
	
	remove(*(itTracker), v);
    }
    
// The function to be cached
    V(*_fn)(const K & , V);
// Maximum number of key-value pairs to be retained
    const size_t _capacity;

// Key access history
    key_tracker_type _key_tracker;
// Key-to-value lookup
    key_to_value_type _key_to_value;
    unsigned levelMinusMinus;
};

#endif //end lru_stl
