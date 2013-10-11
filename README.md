Copyright: Alireza Haghdoost
University of Minnesota: Twin Cities

sim-ideal
=========
Ideal multi-level cache simulator. This simulator has been implemented for the performance mesurement and design study of cache replacement policies.

Implemented Replacement policies
=========
- lru-stl: Based on DRAM environment. Every 30 seconds, flush all dirty pages from cache to storage.
- Page-MIN
- Block-MIN
- OWBP
- ARC
- lru-ziqi: Based on PRAM environment. If the victim is clean, evict it. If the victim is dirty, evict consecutive dirty pages whose length is above the threshold. If none is bigger the threshold, evict the first dirty page along with its consecutive dirty ones.
- lru-dynamic: Based on lru-ziqi. But the threshold is dynamic and initialized to 1. If in the cache, consecutive dirty pages' length is bigger than the threshold could be found, add the threshold by 1. If not, cut the threshold into half. 
- lru-dynamicB: Based on lru-ziqi. But the threshold is dynamic and initialized to 1. If in the cache, consecutive dirty pages' length is bigger than the threshold could be found, add the threshold by 1. If not, decrease the threshold by 1. 
- lru-hotCold: Based on lru-dynamic. Split cache into hot zone and cold zone. Hot zone is some percentage of all the pages that near MRU position and cold zone is the remaining percentage of all the pages that near LRU position. When evicting consecutive dirty pages, we flush them all back but only evicting dirty pages on cold zone and keep those pages in hot zone. Put it another way, we flush back those dirty pages in hot zone and change their status from dirty to clean.
- lru-pure: Based on PRAM environment. Different from lru-stl, there no every 30 seconds auto flush back. Only flush back one dirty page on the LRU position, when cache is full.

Input Trace Format
=========
Microsoft Research trace format, published in SNIA IOTTA trace repository 



Compilation Macros
=========
DEBUG : enable debug mode
VERB : enable verbous output in log.txt
REQSIZE : expand Trace request based on the request size (memTrace is bigger in this case)
HITS : collect histogram for PIRD (page level stack distance) and BIRD (block level stack distance)


Development Branches
=========
master: active development branch
rmBoost: remoove boost library and bimap
futureDistinctSet: uses distinct items in the future for future distance metadata value in OWBP algorithm


Limimtations: 
=========
- only 4 caching layers hardcoded 
- all cache layer in the hierachy need to maitain the same page size

TODO:
=========
- bound block min to future sliding window
- block metadata update rate and update time ?
- a request with reqSize > 1 will always have future distance block 1 , Do we need to fix it ? 

Assumptions:
- a request in the trace access only one block. For example if the request want to access 8 pages, all pages blong to the same block

========
Block i Coldness value at time t = number of valid pages in the cache associated with block i 
	that will not receive any hit in future window. 
block i coldness initial value: At the time of block i insertion, the coldness value could either 0 or 1. 
	if page p accessed in the future window, the coldness value is 0, otherwise it would be 1

there might be a chance to see two block with the same coldness value and distance, for this reason I used multiset for maxHeap. 
However, maxHeapAtoms should maitain a range not only a operator. 
		
if( no future page hit)
		
	block miss , page miss

		coldness = 1;

	block hit, page hit
		hit on previously cold page 
			coldness (nochange)
			no update on coldness (My hypothesis is that this case will not happen any more). this case means that a page is initially cold and after some times
				convert to hot page. However my argument is that if the page is cold, it will not translate to hot page because the containing block will 
				kicked off the write buffer in advance. Therefore, there would be a block miss for the second reference that we assume it can translate cold 
				page to hot page. 
				This condition is very critical if my hyphotesis turns out to be true, It means that the future window size is accurate. 
				if a page does not show up in the future window, it determined as cold and the supporting block will kicked out. 
				any way, I need to validate this hypothesis. 
				The ultimate goal is to be able to detect if current cold page was originaly cold
		
		hit on previously Hot page
			-- coldness
		
	block hit, page miss
		++ codlness
		
example:
	valid_page	cold_page	hot_page	distance
	20		8		12		100
	25		6		19		210
	15		5		10		107
