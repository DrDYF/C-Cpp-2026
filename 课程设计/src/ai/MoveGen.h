#pragma once
#include "core/Board.h"
#include "Evaluator.h"
#include <vector>

struct ScoredMove {
    Point move;
    int score;
    
    bool operator<(const ScoredMove& other) const {
        return score > other.score;  // 降序
    }
};

class MoveGenerator {
public:
    MoveGenerator(Evaluator& evaluator);
    
    // 生成候选着法并排序
    std::vector<Point> generateMoves(const Board& board, Stone color, int maxCount = 16);
    
private:
    bool hasNeighbor(const Board& board, Point p, int radius = 2);
    
    Evaluator& evaluator_;
};
