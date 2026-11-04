#pragma once
#include "core/Board.h"
#include "Pattern.h"

class Evaluator {
public:
    Evaluator();
    
    // 评估局面分数（从 aiColor 视角，正分表示优势）
    int evaluate(const Board& board, Stone aiColor);
    
    // 快速评估某个点的价值（用于着法排序）
    int evaluateMove(const Board& board, Point p, Stone color);
    
private:
    int evaluateColor(const Board& board, Stone color);
    void analyzeLine(const Board& board, Point p, int dx, int dy, 
                     Stone color, int& myScore, int& oppScore);
    
    PatternRecognizer pattern_;
};
