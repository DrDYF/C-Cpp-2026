#pragma once
#include "core/Board.h"
#include <cstdint>

struct TTEntry {
    uint64_t hash;
    int depth;
    int score;
    Point bestMove;
    
    enum Flag : uint8_t {
        EMPTY = 0,
        EXACT = 1,
        ALPHA = 2,
        BETA = 3
    } flag;
};

class TranspositionTable {
public:
    TranspositionTable(size_t sizeMB = 64);
    ~TranspositionTable();
    
    void clear();
    void store(uint64_t hash, int depth, int score, Point bestMove, TTEntry::Flag flag);
    bool probe(uint64_t hash, int depth, int& score, Point& bestMove, TTEntry::Flag& flag);
    
private:
    TTEntry* table_;
    size_t size_;
};
