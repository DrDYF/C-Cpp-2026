#include "AIEngine.h"

AIEngine::AIEngine()
    : moveGen_(evaluator_)
    , searcher_(evaluator_, moveGen_)
    , threatSearcher_(evaluator_) {
}

SearchResult AIEngine::think(const Board& board, int timeLimitMs) {
    Stone currentPlayer = board.getCurrentPlayer();
    
    // 第一步：随机在中心区域
    if (board.getHistory().empty()) {
        return SearchResult{Point(7, 7), 0, 0, 0};
    }
    
    // 第二步：尝试 VCF 算杀（M5）
    Board tempBoard = board;
    Point vcfMove;
    if (threatSearcher_.findVCF(tempBoard, currentPlayer, 10, vcfMove)) {
        return SearchResult{vcfMove, 999999, 0, 0};
    }
    
    // 第三步：常规搜索
    return searcher_.search(board, currentPlayer, timeLimitMs);
}
