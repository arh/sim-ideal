Copyright: Alireza Haghdoost
University of Minnesota

sim-ideal
=========

TODO: 
- update bimap on inserion
- modify CalcFuturePageRef to include distinct distance between adjacent future pages writes 
- check initial block coldness value is <= 1


========
Block i Coldness value at time t = number of valid pages in the cache associated with block i 
	that will not receive any hit in future window. 
block i coldness initial value: At the time of block i insertion, the coldness value could either 0 or 1. 
	if page p accessed in the future window, the coldness value is 0, otherwise it would be 1