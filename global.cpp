#include "global.h"

void exitNow(unsigned code){
	exit(code);
}

cacheAtom cacheAll(const uint64_t & key, cacheAtom new_value)
{
	return new_value;
}
