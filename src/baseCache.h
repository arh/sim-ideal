
#ifndef BASECACHE
#define BASECACHE

template <typename K, typename V> class TestCache
{
public:
    virtual uint32_t access(const K &k  , V &value, uint32_t status) = 0;
    virtual void remove(const K &k) = 0;
};

#endif
