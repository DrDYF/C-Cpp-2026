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
    for(int y=0;y<BoardConst::SIZE;++y) for(int x=0;x<BoardConst::SIZE;++x) {
        Point p(x,y);
        if(!board.isCandidate(p) || board.at(p)!=Stone::Empty) continue;
        const auto pair=evaluator_.evaluateMovePair(board,p);
        if(scores) (*scores)[p.y*15+p.x]=pair;
        const int attack=pair[own], defense=pair[1-own];
        int tier;
        bool retainFork=false;
        if(mode==MoveTierMode::LegacyScore) {
            tier=attack>=ShapeScore::FIVE ? 0 : defense>=ShapeScore::FIVE ? 1 :
                 defense>=ShapeScore::BLOCKED_FOUR ? 2 : attack>=ShapeScore::BLOCKED_FOUR ? 3 : 4;
        } else {
            const auto threats=evaluator_.moveThreats(board,p);
            tier=threats[own].five ? 0 : threats[1-own].five ? 1 :
                 threats[1-own].fours>0 ? 2 : threats[own].fours>0 ? 3 : 4;
            // 同方向有两个胜点，或不同方向各有一个胜点。
            retainFork=threats[own].openFours>0 || threats[own].fours>1;
        }
        int score=attack+defense*11/10;
        score=score*4+28-(std::abs(p.x-7)+std::abs(p.y-7));
        scored.push_back({p,score,tier,retainFork});
    }
    std::stable_sort(scored.begin(),scored.end());

    std::vector<Point> result;
    result.reserve(std::min(maxCount,static_cast<int>(scored.size()))+4);
    for(const ScoredMove& move:scored) {
        // Never prune a direct win, a block of an immediate win, or a block of
        // an opponent four. Also retain our moves that make two winning points.
        if(static_cast<int>(result.size())<maxCount || move.tier<=2 || move.retainFork)
            result.push_back(move.move);
    }
    return result;
}
