#include "MoveGen.h"
#include <algorithm>
#include <array>
#include <cstdlib>

MoveGenerator::MoveGenerator(Evaluator& evaluator):evaluator_(evaluator) {}
std::vector<Point> MoveGenerator::generateMoves(const Board& board,Stone color,int maxCount) {
    if(maxCount<=0 || board.checkWin()!=Stone::Empty) return {};
    if(board.getHistory().empty()) return {Point(7,7)};
    // Mark the neighborhood once instead of scanning 25 neighbors per empty square.
    std::array<bool,225> nearby{};
    for(Point stone:board.getHistory()) for(int dy=-2;dy<=2;++dy) for(int dx=-2;dx<=2;++dx) {
        Point p(stone.x+dx,stone.y+dy);
        if(p.isValid()) nearby[p.y*15+p.x]=true;
    }
    std::vector<ScoredMove> candidates;
    const int own=color==Stone::Black?0:1;
    for(int y=0;y<15;++y) for(int x=0;x<15;++x) {
        Point p(x,y);
        if(!nearby[y*15+x] || board.at(p)!=Stone::Empty) continue;
        auto pair=evaluator_.evaluateMovePair(board,p);
        int attack=pair[own],defense=pair[1-own];
        int score=attack+defense*11/10;
        if(attack>=ShapeScore::FIVE) score=100000000;
        else if(defense>=ShapeScore::FIVE) score=90000000;
        score=score*4+28-(std::abs(x-7)+std::abs(y-7));
        candidates.push_back({p,score});
    }
    std::stable_sort(candidates.begin(),candidates.end());
    std::vector<Point> result;
    int count=std::min(maxCount,static_cast<int>(candidates.size()));
    for(int i=0;i<count;++i) result.push_back(candidates[i].move);
    return result;
}
