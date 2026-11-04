#include "TranspositionTable.h"
#include <algorithm>

TranspositionTable::TranspositionTable(size_t sizeMB)
    :table_(std::max<size_t>(1,sizeMB*1024*1024/sizeof(TTEntry))) {}
void TranspositionTable::clear() { std::fill(table_.begin(),table_.end(),TTEntry{}); }
void TranspositionTable::store(uint64_t hash,int depth,int score,Point move,TTEntry::Flag flag,int ply) {
    TTEntry& entry=table_[hash%table_.size()];
    if(entry.flag!=TTEntry::EMPTY && entry.hash==hash && entry.depth>depth) return;
    if(score>=SearchScore::MateThreshold) score+=ply;
    else if(score<=-SearchScore::MateThreshold) score-=ply;
    entry.hash=hash; entry.depth=depth; entry.score=score; entry.bestMove=move; entry.flag=flag;
}
bool TranspositionTable::probe(uint64_t hash,int depth,int& score,Point& move,TTEntry::Flag& flag,int ply) const {
    const TTEntry& entry=table_[hash%table_.size()];
    if(entry.flag==TTEntry::EMPTY || entry.hash!=hash || entry.depth<depth) return false;
    score=entry.score;
    if(score>=SearchScore::MateThreshold) score-=ply;
    else if(score<=-SearchScore::MateThreshold) score+=ply;
    move=entry.bestMove; flag=entry.flag;
    return true;
}
