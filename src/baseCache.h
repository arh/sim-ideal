
#ifndef BASECACHE
#define BASECACHE

template <typename K, typename V> class TestCache
{
public:
	virtual uint32_t access(const K &k  , V &value, uint32_t status) = 0;
///ziqi: disable virtual remove(), because lru_ziqi.h needs a different remove() with more parameters
	///virtual void remove(const K &k) = 0;
};

#endif
