#pragma once

#include "ai/MoveGen.h"
#include <algorithm>
#include <cstdlib>

inline std::vector<Point> collectQuiescentScores(const Board& board,Evaluator& evaluator,
                                                 std::array<std::array<int,2>,225>& scores) {
    std::vector<Point> candidates;
    candidates.reserve(225);
    for(int y=0;y<BoardConst::SIZE;++y) for(int x=0;x<BoardConst::SIZE;++x) {
        const Point p(x,y);
        if(!board.isCandidate(p) || board.at(p)!=Stone::Empty) continue;
        scores[y*15+x]=evaluator.evaluateMovePair(board,p);
        candidates.push_back(p);
    }
    return candidates;
}

inline void sortQuiescentMoves(std::vector<ScoredMove>& forcing,const std::array<std::array<int,2>,225>& scores,int own) {
    auto oldTier=[&](Point p) {
        const auto& pair=scores[p.y*15+p.x];
        return pair[own]>=ShapeScore::FIVE ? 0 : pair[1-own]>=ShapeScore::FIVE ? 1 :
               pair[1-own]>=ShapeScore::BLOCKED_FOUR ? 2 : pair[own]>=ShapeScore::BLOCKED_FOUR ? 3 : 4;
    };
    auto distance=[](Point p) { return std::abs(p.x-7)+std::abs(p.y-7); };
    // 还原原全量排序留下的同分顺序：先旧分层，再中心距，最后保留格号顺序。
    std::stable_sort(forcing.begin(),forcing.end(),[&](const ScoredMove& a,const ScoredMove& b) {
        if(a.score!=b.score) return a.score>b.score;
        const int ta=oldTier(a.move), tb=oldTier(b.move);
        if(ta!=tb) return ta<tb;
        return distance(a.move)<distance(b.move);
    });
}
