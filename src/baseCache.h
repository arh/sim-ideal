
#ifndef BASECACHE
#define BASECACHE

#include "global.h"


template <typename K, typename V> class TestCache
{
public:
	virtual uint32_t access(const K &k  , V &value, uint32_t status) = 0;
///ziqi: disable virtual remove(), because lru_ziqi.h needs a different remove() with more parameters
	///virtual void remove(const K &k) = 0;

};

class AbsCache {
public:
	virtual reqPacket access( const reqPacket packet ) = 0;
	// Cache level-- in the hierarchy
	bool (*admission)(const cacheAtom);
	cacheAtom (*reqForNext)(const cacheAtom);
	size_t capacity;
	unsigned levelMinusMinus;
	
	AbsCache(
		bool (*f1)(const cacheAtom),
		cacheAtom (*f2)(const cacheAtom),
		size_t c,
		unsigned levelMinus
	) : admission(f1), reqForNext(f2) , capacity(c), levelMinusMinus(levelMinus)	{}
};

#endif
