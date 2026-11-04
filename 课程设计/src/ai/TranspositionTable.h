#pragma once
#include "core/Board.h"
#include <cstdint>
#include <vector>

namespace SearchScore {
constexpr int Mate=100000000;
constexpr int MateThreshold=Mate-1000;
constexpr int Infinity=200000000;
}
struct TTEntry {
    uint64_t hash=0;
    int depth=0, score=0;
    Point bestMove;
    enum Flag : uint8_t { EMPTY, EXACT, ALPHA, BETA } flag=EMPTY;
};
class TranspositionTable {
public:
    explicit TranspositionTable(size_t sizeMB=64);
    void clear();
    void store(uint64_t hash,int depth,int score,Point move,TTEntry::Flag flag,int ply=0);
    bool probe(uint64_t hash,int depth,int& score,Point& move,TTEntry::Flag& flag,int ply=0) const;
private:
    std::vector<TTEntry> table_;
};
