#include "stats.h"
#include "global.h"

using namespace std; 
extern StatsDS _gStats; 

void collectStat( uint32_t newFlags){
	
	++ _gStats.l1Ref;

	if(newFlags	&	PAGEHIT){
		++ _gStats.l1PageHit;
		assert( newFlags & BLKHIT);
		assert( !(newFlags & PAGEMISS));
	}
	if(newFlags	&	PAGEMISS)
		++ _gStats.l1PageMiss;
	if(newFlags	&	BLKHIT){
		++ _gStats.l1BlockHit;
		assert( !(newFlags & BLKMISS) );
	}
	if(newFlags	&	BLKMISS)
		++ _gStats.l1BlockMiss;
	if(newFlags	&	EVICT)
		++ _gStats.l1BlockEvict;
}