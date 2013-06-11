#ifndef STATS
#define STATS
#include <stdint.h>
#include <string>


class Stat
{
private:
    uint64_t counter;
    std::string name;
public:
    Stat() {
        counter = 0;
    }
    Stat(uint64_t value) {
        counter = value;
    }
    Stat(const char statname[]) {
        name = std::string(statname);
        counter = 0;
    }
    inline bool operator==(const Stat &other) const {
        return (counter == other.counter); //this one is faster
    }
    inline bool operator++() {  // like ++ statA
        return (++ counter);
    }
    std::string print() {
        return name.append(",\t").append( std::to_string((unsigned long long) counter)  );
    }
    uint64_t getCounter() {
        return counter;
    }

};

void collectStat( int level, uint32_t newFlags);

void printStats();

class StatsDS
{
private:
    uint16_t returnIndex;
public:
    Stat Ref; //0

    Stat PageRead; //1
    Stat PageWrite; //2

    //read stats
    Stat PageReadHit; //3
    Stat PageReadMiss;
    Stat BlockReadHit;
    Stat BlockReadMiss;

    // write stats
    Stat PageWriteHit; //7
    Stat PageWriteMiss;
    Stat BlockWriteHit;
    Stat BlockWriteMiss;
    Stat BlockEvict; //11

    Stat Cold2Cold; //12
    Stat Cold2Hot;
    ///ziqi
    Stat DirtyPage; //14
    Stat SeqEviction;
    Stat LessSeqEviction;

    StatsDS()
        : Ref("Total References")
        , PageRead("Total Reads")
        , PageWrite("Total Writes")
        , PageReadHit("Page Read Hit")
        , PageReadMiss("Page Read Miss")
        , BlockReadHit("Block Read Hit")
        , BlockReadMiss("Block Read Miss")

        , PageWriteHit("Page Write Hit")
        , PageWriteMiss("Page Write Miss")
        , BlockWriteHit("Block Write Hit")
        , BlockWriteMiss("Block Write Miss")
        , BlockEvict("Block Evict")
        , Cold2Cold("Cold2Cold")
        , Cold2Hot("Cold2Hot")
        , DirtyPage("DirtyPage") ///ziqi
        , SeqEviction("SeqEviction") ///ziqi
        , LessSeqEviction("LessSeqEviction") { ///ziqi
        returnIndex = 0;
    }
    Stat *next() {
        switch(returnIndex) {
        case 0:
            ++returnIndex;
            return &Ref;
        case 1:
            ++returnIndex;
            return &PageRead;
        case 2:
            ++returnIndex;
            return &PageWrite;
        case 3:
            ++returnIndex;
            return &PageReadHit;
        case 4:
            ++returnIndex;
            return &PageReadMiss;
        case 5:
            ++returnIndex;
            return &BlockReadHit;
        case 6:
            ++returnIndex;
            return &BlockReadMiss;
        case 7:
            ++returnIndex;
            return &PageWriteHit;
        case 8:
            ++returnIndex;
            return &PageWriteMiss;
        case 9:
            ++returnIndex;
            return &BlockWriteHit;
        case 10:
            ++returnIndex;
            return &BlockWriteMiss;
        case 11:
            ++returnIndex;
            return &BlockEvict;
        case 12:
            ++returnIndex;
            return &Cold2Cold;
        case 13:
            ++returnIndex;
            return &Cold2Hot;
        case 14:
            ++returnIndex;
            return &DirtyPage; ///ziqi
        case 15:
            ++returnIndex;
            return &SeqEviction; ///ziqi
        case 16:
            ++returnIndex;
            return &LessSeqEviction; ///ziqi
        default:
            return NULL;
        }
    }
};

class L1StatsDS : public StatsDS
{


};
#endif
