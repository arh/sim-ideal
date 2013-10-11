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

#ifndef DARC_H
#define DARC_H

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

extern int totalEvictedCleanPages;

extern int totalNonSeqEvictedDirtyPages;

// Class providing fixed-size (by number of records)
// LRU-replacement cache of a function with signature
// V f(K)
template <typename K, typename V>
class DARC : public TestCache<K, V>
{
public:
// Key access history, most recent at back
    typedef list<K> key_tracker_type;
// Key to value and key history iterator
    typedef map
    < K, pair<V, typename key_tracker_type::iterator> > 	key_to_value_type;
// Constuctor specifies the cached function and
// the maximum number of records to be stored.
    DARC(
        V(*f)(const K & , V),
        size_t c,
        unsigned levelMinus
    ) : _fn(f) , _capacity(c), levelMinusMinus(levelMinus)  {
        ///ARH: Commented for single level cache implementation
//         assert ( _capacity!=0 );
    }
    // Obtain value of the cached function for k

    uint32_t access(const K &k  , V &value, uint32_t status) {
      	PRINTV(logfile << endl;);	
        ///ziqi: p denotes the length of t1 and (_capacity - p) denotes the lenght of t2
	static int p=0;
	
        assert((t1.size() + t2.size()) <= _capacity);
	assert((t1.size() + t2.size() + b1.size() + b2.size()) <= 2*_capacity);
	assert((t1.size() + b1.size()) <= _capacity);
	assert((t2.size() + b2.size()) <= 2*_capacity);
        assert(_capacity != 0);
        PRINTV(logfile << "Access key: " << k << endl;);

///ziqi: if request is write, mark the page status as DIRTY
        if(status & WRITE) {
            status |= DIRTY;
            value.updateFlags(status);
            //cout<<"flags are "<<value.getFlags()<<endl;
            //const V v1 = _fn(k, value);
            //insertDirtyPage(k, v1);
        }

// Attempt to find existing record
        const typename key_to_value_type::iterator it_t1	= t1.find(k);
	const typename key_to_value_type::iterator it_t2	= t2.find(k);
	const typename key_to_value_type::iterator it_b1	= b1.find(k);
	const typename key_to_value_type::iterator it_b2	= b2.find(k);
        //const typename key_to_value_type::iterator itNew	= _key_to_value.find(k);
    
	///ziqi: DARC Case I: x hit in t1, then move x to t1 if it's a read, or to t2 if it's a write
        if((it_t1 != t1.end()) && (it_t2 == t2.end())) {
	    assert(!((it_t1 != t1.end()) && (it_t2 != t2.end())));
	    if(it_t1->second.first.getReq().flags & DIRTY) {
	      PRINTV(logfile << "Case I Write hit on t1 " << k << endl;);	
	      t1.erase(it_t1);
	      t1_key.remove(k);
	      assert(t1.size() < _capacity);
	      const V v = _fn(k, value);
	      // Record k as most-recently-used key
	      typename key_tracker_type::iterator itNew
	      = t2_key.insert(t2_key.end(), k);
	      // Create the key-value entry,
	      // linked to the usage record.
	      t2.insert(make_pair(k, make_pair(v, itNew)));
	      PRINTV(logfile << "Case I insert dirty key to t2: " << k << "** t1 size: "<< t1.size()<< ", t2 size: "<< t2.size() <<endl;);	      
	      return (status | PAGEHIT | BLKHIT);
	    }	    
	    else {
	      PRINTV(logfile << "Case I Read hit on t1 " << k << endl;);
	      t1.erase(it_t1);
	      t1_key.remove(k);
	      assert(t1.size() < _capacity);
	      const V v = _fn(k, value);
	      // Record k as most-recently-used key
	      typename key_tracker_type::iterator itNew
	      = t1_key.insert(t1_key.end(), k);
	      // Create the key-value entry,
	      // linked to the usage record.
	      t1.insert(make_pair(k, make_pair(v, itNew)));
	      PRINTV(logfile << "Case I insert clean key to t1: " << k << "** t1 size: "<< t1.size()<< ", t2 size: "<< t2.size() <<endl;);
	      return (status | PAGEHIT | BLKHIT);	      
	    }
         
        }
        
        ///ziqi: DARC Case II: x hit in t2, then move x to MRU of t2
        if((it_t2 != t2.end()) && (it_t1 == t1.end())) {
	    assert(!((it_t1 != t1.end()) && (it_t2 != t2.end())));
	    
	    value.updateFlags(status | (it_t2->second.first.getReq().flags & DIRTY));
	    
	    t2.erase(it_t2);
	    t2_key.remove(k);
	    assert(t2.size() < _capacity);
	    const V v = _fn(k, value);
	    // Record k as most-recently-used key
	    typename key_tracker_type::iterator itNew = t2_key.insert(t2_key.end(), k);
	    // Create the key-value entry,
	    // linked to the usage record.
	    t2.insert(make_pair(k, make_pair(v, itNew)));
	    PRINTV(logfile << "Case II insert key to t2: " << k << "** t1 size: "<< t1.size()<< ", t2 size: "<< t2.size() <<endl;);	      
	    return (status | PAGEHIT | BLKHIT);
	          
	}      
        ///ziqi: ARC Case III: x hit in b1, then enlarge t1, and move x from b1 to t1 if it's a read or to t2 if it's a write
        else if(it_b1 != b1.end()){
	    ///ziqi: delta denotes the step in each ADAPTATION
	    int delta;
	    
	    PRINTV(logfile << "Case III Hit on b1: " << k << endl;);
	    //if(b1.size() >= b2.size())
	      delta = 1;
	    //else
	      //delta = int(b2.size()/b1.size());
	    
	    PRINTV(logfile << "ADAPTATION: p increases from " << p << " to ";);
	    
	    if((p+delta) > int(_capacity))
	      p = _capacity;
	    else
	      p = p+delta;
	   
	    PRINTV(logfile << p << endl;);
	    
	    const V v = _fn(k, value);
	    
	    REPLACE(k, v, p);
	    
	    b1.erase(it_b1);
	    b1_key.remove(k);
	    
	    PRINTV(logfile << "Case III evicting b1 " << k <<  endl;);
	    
	    if(status & DIRTY) {	    
	      // Record k as most-recently-used key
	      typename key_tracker_type::iterator itNew = t2_key.insert(t2_key.end(), k);
	      // Create the key-value entry,
	      // linked to the usage record.
	      t2.insert(make_pair(k, make_pair(v, itNew)));
	      PRINTV(logfile << "Case III insert key to t2: " << k << "** t1 size: "<< t1.size()<< ", t2 size: "<< t2.size() <<endl;);
	    }
	    else {
	       // Record k as most-recently-used key
	      typename key_tracker_type::iterator itNew = t1_key.insert(t1_key.end(), k);
	      // Create the key-value entry,
	      // linked to the usage record.
	      t1.insert(make_pair(k, make_pair(v, itNew)));
	      PRINTV(logfile << "Case III insert key to t1: " << k << "** t1 size: "<< t1.size()<< ", t2 size: "<< t2.size() <<endl;);
	      
	      if(t1.size() + b1.size() > _capacity) {	      
		if(b1.size() > 0) {
		  typename key_tracker_type::iterator itLRU = b1_key.begin();
		  assert(itLRU != b1_key.end());		
		  typename key_to_value_type::iterator it = b1.find(*itLRU);		
		  b1.erase(it);
		  b1_key.remove(*itLRU);
		  PRINTV(logfile << "Case III evicting b1 " << *itLRU <<  endl;);
		}
		else {
		  typename key_tracker_type::iterator itLRU = t1_key.begin();
		  assert(itLRU != t1_key.end());		
		  typename key_to_value_type::iterator it = t1.find(*itLRU);		
		  t1.erase(it);
		  t1_key.remove(*itLRU);
		  totalEvictedCleanPages++;
		  PRINTV(logfile << "Case III evicting t1 without flushing back to DiskSim input trace " << *itLRU <<  endl;);
		}
	      }
	    }
	    return (status | PAGEMISS);		    
	}
	///ziqi: ARC Case IV: x hit in b2, then enlarge t2, and move x from b2 to t1 if it's a read or to t2 if it's a write
	else if(it_b2 != b2.end()){
	    ///ziqi: delta denotes the step in each ADAPTATION
	    int delta;
	    
	    PRINTV(logfile << "Case IV Hit on b2: " << k << endl;);	
	    if(b2.size() >= b1.size())
	      delta = 4;
	    else
	      delta = int(4*b1.size()/b2.size());
	    
	    PRINTV(logfile << "ADAPTATION: p decreases from " << p << " to ";);
	    
	    if((p-delta) > 0)
	      p = p-delta;
	    else
	      p = 0;
	   
	    PRINTV(logfile << p << endl;);
	    
	    const V v = _fn(k, value);
	    
	    REPLACE(k, v, p);
	    
	    b2.erase(it_b2);
	    b2_key.remove(k);
	    
	    PRINTV(logfile << "Case IV evicting b2 " << k <<  endl;);

	    if(status & DIRTY) {	    
	      // Record k as most-recently-used key
	      typename key_tracker_type::iterator itNew = t2_key.insert(t2_key.end(), k);
	      // Create the key-value entry,
	      // linked to the usage record.
	      t2.insert(make_pair(k, make_pair(v, itNew)));
	      PRINTV(logfile << "Case IV insert key to t2: " << k << "** t1 size: "<< t1.size()<< ", t2 size: "<< t2.size() <<endl;);
	    }
	    else {
	       // Record k as most-recently-used key
	      typename key_tracker_type::iterator itNew = t1_key.insert(t1_key.end(), k);
	      // Create the key-value entry,
	      // linked to the usage record.
	      t1.insert(make_pair(k, make_pair(v, itNew)));
	      PRINTV(logfile << "Case IV insert key to t1: " << k << "** t1 size: "<< t1.size()<< ", t2 size: "<< t2.size() <<endl;);
	      
	      if(t1.size() + b1.size() > _capacity) {	      
		if(b1.size() > 0) {
		  typename key_tracker_type::iterator itLRU = b1_key.begin();
		  assert(itLRU != b1_key.end());		
		  typename key_to_value_type::iterator it = b1.find(*itLRU);		
		  b1.erase(it);
		  b1_key.remove(*itLRU);
		  PRINTV(logfile << "Case IV evicting b1 " << *itLRU <<  endl;);
		}
		else {
		  typename key_tracker_type::iterator itLRU = t1_key.begin();
		  assert(itLRU != t1_key.end());		
		  typename key_to_value_type::iterator it = t1.find(*itLRU);		
		  t1.erase(it);
		  t1_key.remove(*itLRU);
		  totalEvictedCleanPages++;
		  PRINTV(logfile << "Case IV evicting t1 without flushing back to DiskSim input trace " << *itLRU <<  endl;);
		}
	      }
	    }
	    return (status | PAGEMISS);		    	    
	} 
	///ziqi: ARC Case V: x is cache miss  
        else {
            PRINTV(logfile << "Case V miss on key: " << k << endl;);
	    
	    if((t1.size() + b1.size()) == _capacity) {	    
	      if(t1.size() < _capacity) {
		typename key_tracker_type::iterator itLRU = b1_key.begin();
		assert(itLRU != b1_key.end());
		typename key_to_value_type::iterator it = b1.find(*itLRU);
		b1.erase(it);
	        b1_key.remove(*itLRU);
		
		const V v = _fn(k, value);
		
		REPLACE(k, v, p);
	      }
	      else {
		typename key_tracker_type::iterator itLRU = t1_key.begin();
		assert(itLRU != t1_key.end());
		
		typename key_to_value_type::iterator it = t1.find(*itLRU);
		
		const V v = _fn(k, value);
		
		if(it->second.first.getReq().flags & DIRTY) {	
		  ///ziqi: DiskSim format Request_arrival_time Device_number Block_number Request_size Request_flags
		  ///ziqi: Device_number is set to 1. About Request_flags, 0 is for write and 1 is for read
		  PRINTV(DISKSIMINPUTSTREAM << setfill(' ')<<left<<fixed<<setw(25)<<v.getReq().issueTime<<left<<setw(8)<<"0"<<left<<fixed<<setw(12)<<it->second.first.getReq().fsblkno<<left<<fixed<<setw(8)<<it->second.first.getReq().reqSize<<"0"<<endl;);	
		  
		  // Erase both elements to completely purge record	
		  PRINTV(logfile << "Case V evicting dirty key " << k <<  endl;);
		  totalNonSeqEvictedDirtyPages++;
		  PRINTV(logfile << "Case V Key dirty bit status: " << bitset<10>(it->second.first.getReq().flags) << endl;);
		  t1.erase(it);
		  t1_key.remove(*itLRU);
		  
		  PRINTV(logfile << "Case V t1 size: " << t1.size() <<endl;);
		}
		else {
		  PRINTV(logfile << "Case V evicting clean key without flushing back to DiskSim input trace " << k <<  endl;);
		  totalEvictedCleanPages++;
		  PRINTV(logfile << "Case V Key clean bit status: " << bitset<10>(it->second.first.getReq().flags) << endl;);

		  t1.erase(it);
		  t1_key.remove(*itLRU);
		  PRINTV(logfile << "Case V t1 size: " << t1.size() <<endl;);
		}
	      }
	    }
	    else if((t1.size() + b1.size()) < _capacity) {	    
	      if((t1.size() + t2.size() + b1.size() + b2.size()) >= _capacity) {
		if((t1.size() + t2.size() + b1.size() + b2.size()) == 2 * _capacity) {
		  typename key_tracker_type::iterator itLRU = b2_key.begin();
		  assert(itLRU != b2_key.end());
		  typename key_to_value_type::iterator it = b2.find(*itLRU);
		  b2.erase(it);
		  b2_key.remove(*itLRU);
		}
		
		const V v = _fn(k, value);
		
		REPLACE(k, v, p);		
	      }
	    }
	   
	    const V v = _fn(k, value);
	    
	    if(status & DIRTY) {	    
	      // Record k as most-recently-used key
	      typename key_tracker_type::iterator itNew = t2_key.insert(t2_key.end(), k);
	      // Create the key-value entry,
	      // linked to the usage record.
	      t2.insert(make_pair(k, make_pair(v, itNew)));
	      PRINTV(logfile << "Case V insert key to t2: " << k << "** t1 size: "<< t1.size()<< ", t2 size: "<< t2.size() <<endl;);
	    }
	    else {
	       // Record k as most-recently-used key
	      typename key_tracker_type::iterator itNew = t1_key.insert(t1_key.end(), k);
	      // Create the key-value entry,
	      // linked to the usage record.
	      t1.insert(make_pair(k, make_pair(v, itNew)));
	      PRINTV(logfile << "Case V insert key to t1: " << k << "** t1 size: "<< t1.size()<< ", t2 size: "<< t2.size() <<endl;);	      
	    }
	    
	    return (status | PAGEMISS);
        }
        
        return 0;
             
    } //end operator access
    
    ///ziqi: ARC subroutine
    void REPLACE(const K &k, const V &v, int p) {
      typename key_to_value_type::iterator it = b2.find(k);

      if((t1.size() > 0) && ((t1.size() > p) || ((it != b2.end()) && (t1.size() == p)))) {
	typename key_tracker_type::iterator itLRU = t1_key.begin();
	assert(itLRU != t1_key.end());
	typename key_to_value_type::iterator itLRUValue = t1.find(*itLRU);
	
        if(itLRUValue->second.first.getReq().flags & DIRTY) {	
          ///ziqi: DiskSim format Request_arrival_time Device_number Block_number Request_size Request_flags
	  ///ziqi: Device_number is set to 1. About Request_flags, 0 is for write and 1 is for read
	  PRINTV(DISKSIMINPUTSTREAM << setfill(' ')<<left<<fixed<<setw(25)<<v.getReq().issueTime<<left<<setw(8)<<"0"<<left<<fixed<<setw(12)<<itLRUValue->second.first.getReq().fsblkno<<left<<fixed<<setw(8)<<itLRUValue->second.first.getReq().reqSize<<"0"<<endl;);	
		  	  
	  PRINTV(logfile << "REPLACE evicting dirty key " << *itLRU <<  endl;);
	  totalNonSeqEvictedDirtyPages++;
	  PRINTV(logfile << "REPLACE Key dirty bit status: " << bitset<10>(itLRUValue->second.first.getReq().flags) << endl;);
	  t1.erase(itLRUValue);
	  t1_key.remove(*itLRU);
		  
	  PRINTV(logfile << "REPLACE t1 size: " << t1.size() <<endl;);
	}
	else {
	  PRINTV(logfile << "REPLACE evicting clean key without flushing back to DiskSim input trace " << *itLRU <<  endl;);
	  totalEvictedCleanPages++;
	  PRINTV(logfile << "REPLACE Key clean bit status: " << bitset<10>(itLRUValue->second.first.getReq().flags) << endl;);

	  t1.erase(itLRUValue);
	  t1_key.remove(*itLRU);
	  PRINTV(logfile << "REPLACE t1 size: " << t1.size() <<endl;);
        }
        
	// Record k as most-recently-used key
	typename key_tracker_type::iterator itNew = b1_key.insert(b1_key.end(), k);
	// Create the key-value entry,
	// linked to the usage record.
	b1.insert(make_pair(k, make_pair(v, itNew)));   
	PRINTV(logfile << "REPLACE insert key to b1: " << k <<  "** b1 size: "<< b1.size() << endl;);
      }
      else {
	typename key_tracker_type::iterator itLRU = t2_key.begin();
	assert(itLRU != t2_key.end());
	typename key_to_value_type::iterator itLRUValue = t2.find(*itLRU);
	
        if(itLRUValue->second.first.getReq().flags & DIRTY) {	
          ///ziqi: DiskSim format Request_arrival_time Device_number Block_number Request_size Request_flags
	  ///ziqi: Device_number is set to 1. About Request_flags, 0 is for write and 1 is for read
	  PRINTV(DISKSIMINPUTSTREAM << setfill(' ')<<left<<fixed<<setw(25)<<v.getReq().issueTime<<left<<setw(8)<<"0"<<left<<fixed<<setw(12)<<itLRUValue->second.first.getReq().fsblkno<<left<<fixed<<setw(8)<<itLRUValue->second.first.getReq().reqSize<<"0"<<endl;);	
		  
	  // Erase both elements to completely purge record	
	  PRINTV(logfile << "REPLACE evicting dirty key " << *itLRU <<  endl;);
	  totalNonSeqEvictedDirtyPages++;
	  PRINTV(logfile << "REPLACE Key dirty bit status: " << bitset<10>(itLRUValue->second.first.getReq().flags) << endl;);
	  t2.erase(itLRUValue);
	  t2_key.remove(*itLRU);
		  
	  PRINTV(logfile << "REPLACE t2 size: " << t1.size() <<endl;);
	}
	else {
	  PRINTV(logfile << "REPLACE evicting clean key without flushing back to DiskSim input trace " << *itLRU <<  endl;);
	  totalEvictedCleanPages++;
	  PRINTV(logfile << "REPLACE Key clean bit status: " << bitset<10>(itLRUValue->second.first.getReq().flags) << endl;);

	  t2.erase(itLRUValue);
	  t2_key.remove(*itLRU);
	  PRINTV(logfile << "REPLACE t2 size: " << t1.size() <<endl;);
        }
        
	// Record k as most-recently-used key
	typename key_tracker_type::iterator itNew = b2_key.insert(b2_key.end(), k);
	// Create the key-value entry,
	// linked to the usage record.
	b2.insert(make_pair(k, make_pair(v, itNew)));
	PRINTV(logfile << "REPLACE insert key to b2: " << k <<  "** b2 size: "<< b2.size() << endl;);
	
      }    
    }
        
private:    
    
// The function to be cached
    V(*_fn)(const K & , V);
// Maximum number of key-value pairs to be retained
    const size_t _capacity;

    unsigned levelMinusMinus;
    
    ///ziqi: Key access history
    key_tracker_type t1_key;
    ///ziqi: Key-to-value lookup
    key_to_value_type t1;
    ///ziqi: Key access history
    key_tracker_type t2_key;
    ///ziqi: Key-to-value lookup
    key_to_value_type t2;
    ///ziqi: Key access history
    key_tracker_type b1_key;
    ///ziqi: Key-to-value lookup
    key_to_value_type b1;
    ///ziqi: Key access history
    key_tracker_type b2_key;
    ///ziqi: Key-to-value lookup
    key_to_value_type b2;
};

#endif //end lru_stl
