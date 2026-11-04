#pragma once
#include "core/Board.h"
#include "Evaluator.h"
#include <vector>

struct ScoredMove {
    Point move;
    int score;
    bool operator<(const ScoredMove& other) const { return score>other.score; }
};
class MoveGenerator {
public:
    explicit MoveGenerator(Evaluator& evaluator);
    std::vector<Point> generateMoves(const Board& board,Stone color,int maxCount=16);
private:
    Evaluator& evaluator_;
};
