#include "global.h"
#include "stats.h"

void ExitNow(unsigned code)
{
	printStats();
	exit(code);
}

cacheAtom cacheAll(const uint64_t &key, cacheAtom new_value)
{
	return new_value;
}
