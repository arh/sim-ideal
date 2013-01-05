#ifndef STATS
#define STATS
#include <stdint.h>
#include <string>


class Stat {
private:
	uint64_t counter;
	std::string name;
public:
	Stat(){
		counter = 0;
	}
	Stat(uint64_t value){
		counter = value; 
	}
	inline bool operator==(const Stat & other) const {
		return (counter == other.counter); //this one is faster
	}
	inline bool operator++() {  // like ++ statA
		return (++ counter); 
	}
	std::string print() {
		return name.append(",\t").append(std::string( std::to_string(counter) ) );
	}
};

void collectStat( uint32_t newFlags);

class StatsDS {
public:
	Stat l1PageHit;
	Stat l1PageMiss;
	Stat l1Ref;
	Stat l1BlockHit;
	Stat l1BlockMiss;
	Stat l1BlockEvict;
};
#endif