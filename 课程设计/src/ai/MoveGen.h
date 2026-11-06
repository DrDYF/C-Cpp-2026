#pragma once
#include "core/Board.h"
#include "Evaluator.h"
#include <vector>

struct ScoredMove {
    Point move;
    int score;
    int tier=4;
    bool operator<(const ScoredMove& other) const {
        if(tier!=other.tier) return tier<other.tier;
        return score>other.score;
    }
};
class MoveGenerator {
public:
    explicit MoveGenerator(Evaluator& evaluator);
    std::vector<Point> generateMoves(const Board& board,Stone color,int maxCount=16);
private:
    Evaluator& evaluator_;
};
