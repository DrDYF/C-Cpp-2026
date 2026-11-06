#include "MoveGen.h"
#include <algorithm>
#include <cstdlib>

MoveGenerator::MoveGenerator(Evaluator& evaluator):evaluator_(evaluator) {}

std::vector<Point> MoveGenerator::generateMoves(const Board& board,Stone color,int maxCount) {
    if(maxCount<=0 || board.checkWin()!=Stone::Empty) return {};
    std::vector<ScoredMove> scored;
    scored.reserve(std::min(225,maxCount)+16);
    const int own=color==Stone::Black?0:1;
    for(int y=0;y<BoardConst::SIZE;++y) for(int x=0;x<BoardConst::SIZE;++x) {
        Point p(x,y);
        if(!board.isCandidate(p) || board.at(p)!=Stone::Empty) continue;
        const auto pair=evaluator_.evaluateMovePair(board,p);
        const int attack=pair[own], defense=pair[1-own];
        const bool wins=attack>=ShapeScore::FIVE;
        const bool blocksWin=defense>=ShapeScore::FIVE;
        const bool blocksFour=defense>=ShapeScore::BLOCKED_FOUR;
        const bool makesFour=attack>=ShapeScore::BLOCKED_FOUR;

        // Separate tactical classes. If the opponent has an immediate win,
        // blocking that point outranks a non-winning attack; an immediate win
        // by us outranks every defense.
        const int tier=wins ? 0 : blocksWin ? 1 : blocksFour ? 2 : makesFour ? 3 : 4;
        int score=attack+defense*11/10;
        score=score*4+28-(std::abs(p.x-7)+std::abs(p.y-7));
        scored.push_back({p,score,tier});
    }
    std::stable_sort(scored.begin(),scored.end());

    std::vector<Point> result;
    result.reserve(std::min(maxCount,static_cast<int>(scored.size()))+4);
    for(const ScoredMove& move:scored) {
        // Never prune a direct win, a block of an immediate win, or a block of
        // an opponent four. Quiet candidates remain capped for search breadth.
        if(static_cast<int>(result.size())<maxCount || move.tier<=2)
            result.push_back(move.move);
    }
    return result;
}
