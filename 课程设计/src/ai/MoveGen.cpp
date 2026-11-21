#include "MoveGen.h"
#include <algorithm>
#include <cstdlib>

MoveGenerator::MoveGenerator(Evaluator& evaluator):evaluator_(evaluator) {}

std::vector<Point> MoveGenerator::generateMoves(const Board& board,Stone color,int maxCount,MoveTierMode mode,
                                               std::array<std::array<int,2>,225>* scores) {
    if(maxCount<=0 || board.checkWin()!=Stone::Empty) return {};
    std::vector<ScoredMove> scored;
    scored.reserve(std::min(225,maxCount)+16);
    const int own=color==Stone::Black?0:1;
    evaluator_.forEachCandidateAnalysis(board,[&](Point p,const MoveAnalysis& analysis) {
        if(scores) (*scores)[p.y*15+p.x]={analysis[0].score,analysis[1].score};
        const int attack=analysis[own].score, defense=analysis[1-own].score;
        int tier;
        bool retainFork=false;
        if(mode==MoveTierMode::LegacyScore) {
            tier=attack>=ShapeScore::FIVE ? 0 : defense>=ShapeScore::FIVE ? 1 :
                 defense>=ShapeScore::BLOCKED_FOUR ? 2 : attack>=ShapeScore::BLOCKED_FOUR ? 3 : 4;
        } else {
            tier=analysis[own].five ? 0 : analysis[1-own].five ? 1 :
                 analysis[1-own].fours>0 ? 2 : analysis[own].fours>0 ? 3 : 4;
            // 同方向有两个胜点，或不同方向各有一个胜点。
            retainFork=analysis[own].openFours>0 || analysis[own].fours>1;
        }
        int score=attack+defense*11/10;
        score=score*4+28-(std::abs(p.x-7)+std::abs(p.y-7));
        scored.push_back({p,score,tier,retainFork});
    });
    const auto less=[](const ScoredMove& a,const ScoredMove& b) {
        if(a<b) return true;
        if(b<a) return false;
        return a.move.y*15+a.move.x<b.move.y*15+b.move.x;
    };
    const auto k=std::min<std::size_t>(maxCount,scored.size());
    auto middle=scored.begin()+k;
    std::partial_sort(scored.begin(),middle,scored.end(),less);
    auto end=std::partition(middle,scored.end(),[](const ScoredMove& m){return m.tier<=2 || m.retainFork;});
    std::sort(middle,end,less);
    std::vector<Point> result;
    result.reserve(end-scored.begin());
    for(auto it=scored.begin();it!=end;++it) result.push_back(it->move);
    return result;
}
