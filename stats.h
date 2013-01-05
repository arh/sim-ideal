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
	Stat(const char statname[]){
		name = std::string(statname);
		counter = 0;
	}
	inline bool operator==(const Stat & other) const {
		return (counter == other.counter); //this one is faster
	}
	inline bool operator++() {  // like ++ statA
		return (++ counter); 
	}
	std::string print() {
		return name.append(",\t").append( std::to_string((unsigned long long) counter)  );
	}
};

void collectStat( uint32_t newFlags);

void printStats();

class StatsDS {
private:
	uint16_t returnIndex;
public:
	Stat Ref; //0
	Stat PageHit;
	Stat PageMiss;
	Stat BlockHit;
	Stat BlockMiss;
	Stat BlockEvict; //5
	
	StatsDS()
	: Ref("Total References")
	, PageHit("Page Hit")
	, PageMiss("Page Miss")
	, BlockHit("Block Hit")
	, BlockMiss("Block Miss")
	, BlockEvict("Block Evict")
	{
		returnIndex=0;
	}
	Stat * next(){
		switch(returnIndex){
			case 0:	++returnIndex; return &Ref;
			case 1:	++returnIndex; return &PageHit;
			case 2:	++returnIndex; return &PageMiss;
			case 3:	++returnIndex; return &BlockHit;
			case 4:	++returnIndex; return &BlockMiss;
			case 5:	++returnIndex; return &BlockEvict;
			default:	return NULL;
		}
	}
};

class L1StatsDS : public StatsDS{
	
	
};
#endif
