#pragma once
#include "core/Board.h"
#include "Evaluator.h"

// VCF (Victory by Continuous Four) 算杀
class ThreatSearcher {
public:
    ThreatSearcher(Evaluator& evaluator);
    
    // 尝试找到 VCF 杀棋
    bool findVCF(Board& board, Stone color, int maxDepth, Point& result);
    
private:
    bool vcfSearch(Board& board, Stone attacker, int depth, int maxDepth);
    std::vector<Point> generateThreats(const Board& board, Stone color);
    std::vector<Point> generateDefenses(const Board& board, Stone color);
    
    Evaluator& evaluator_;
};
