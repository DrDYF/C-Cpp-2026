#include "TranspositionTable.h"
#include <cstring>

TranspositionTable::TranspositionTable(size_t sizeMB) {
    size_ = (sizeMB * 1024 * 1024) / sizeof(TTEntry);
    table_ = new TTEntry[size_];
    clear();
}

TranspositionTable::~TranspositionTable() {
    delete[] table_;
}

void TranspositionTable::clear() {
    std::memset(table_, 0, size_ * sizeof(TTEntry));
}

void TranspositionTable::store(uint64_t hash, int depth, int score, Point bestMove, TTEntry::Flag flag) {
    size_t index = hash % size_;
    TTEntry& entry = table_[index];
    
    // 替换策略：深度优先
    if (entry.flag == TTEntry::EMPTY || depth >= entry.depth) {
        entry.hash = hash;
        entry.depth = depth;
        entry.score = score;
        entry.bestMove = bestMove;
        entry.flag = flag;
    }
}

bool TranspositionTable::probe(uint64_t hash, int depth, int& score, Point& bestMove, TTEntry::Flag& flag) {
    size_t index = hash % size_;
    TTEntry& entry = table_[index];
    
    if (entry.flag != TTEntry::EMPTY && entry.hash == hash && entry.depth >= depth) {
        score = entry.score;
        bestMove = entry.bestMove;
        flag = entry.flag;
        return true;
    }
    
    return false;
}
