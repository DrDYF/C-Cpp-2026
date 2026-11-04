#include "MoveGen.h"
#include <algorithm>

MoveGenerator::MoveGenerator(Evaluator& evaluator) 
    : evaluator_(evaluator) {
}

std::vector<Point> MoveGenerator::generateMoves(const Board& board, Stone color, int maxCount) {
    std::vector<ScoredMove> candidates;
    
    // 开局特殊处理：如果棋盘空，返回中心
    if (board.getHistory().empty()) {
        return {Point(7, 7)};
    }
    
    // 遍历所有空位
    for (int y = 0; y < BoardConst::SIZE; ++y) {
        for (int x = 0; x < BoardConst::SIZE; ++x) {
            Point p(x, y);
            if (board.at(p) != Stone::Empty) continue;
            
            // 只考虑已有棋子周围2格内的点
            if (!hasNeighbor(board, p, 2)) continue;
            
            int score = evaluator_.evaluateMove(board, p, color);
            candidates.push_back({p, score});
        }
    }
    
    // 排序
    std::sort(candidates.begin(), candidates.end());
    
    // 取前 N 个
    std::vector<Point> result;
    int count = std::min(maxCount, static_cast<int>(candidates.size()));
    for (int i = 0; i < count; ++i) {
        result.push_back(candidates[i].move);
    }
    
    return result;
}

bool MoveGenerator::hasNeighbor(const Board& board, Point p, int radius) {
    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            if (dx == 0 && dy == 0) continue;
            Point neighbor(p.x + dx, p.y + dy);
            if (neighbor.isValid() && board.at(neighbor) != Stone::Empty) {
                return true;
            }
        }
    }
    return false;
}
