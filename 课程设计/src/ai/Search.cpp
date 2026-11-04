#include "Search.h"
#include "core/Zobrist.h"
#include <algorithm>
#include <chrono>

Searcher::Searcher(Evaluator& evaluator, MoveGenerator& moveGen)
    : evaluator_(evaluator)
    , moveGen_(moveGen)
    , tt_(64)
    , nodes_(0)
    , rootColor_(Stone::Black) {
}

SearchResult Searcher::search(const Board& board, Stone color, int timeLimitMs) {
    nodes_ = 0;
    rootColor_ = color;
    
    auto startTime = std::chrono::steady_clock::now();
    
    Point bestMove(-1, -1);
    int bestScore = -999999999;
    int finalDepth = 0;
    
    // 迭代加深
    for (int depth = 2; depth <= 10; depth += 2) {
        Board tempBoard = board;
        
        auto candidates = moveGen_.generateMoves(tempBoard, color, 16);
        if (candidates.empty()) break;
        
        int alpha = -999999999;
        int beta = 999999999;
        Point currentBest = candidates[0];
        int currentScore = alpha;
        
        for (auto& move : candidates) {
            tempBoard.place(move, color);
            Stone oppColor = (color == Stone::Black) ? Stone::White : Stone::Black;
            int score = -negamax(tempBoard, oppColor, depth - 1, -beta, -alpha);
            tempBoard.undo();
            
            if (score > currentScore) {
                currentScore = score;
                currentBest = move;
            }
            
            alpha = std::max(alpha, score);
            if (alpha >= beta) break;
            
            // 检查时间
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
            if (elapsed >= timeLimitMs) {
                finalDepth = depth;
                goto time_out;
            }
        }
        
        bestMove = currentBest;
        bestScore = currentScore;
        finalDepth = depth;
        
        // 检查时间
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
        if (elapsed >= timeLimitMs) break;
    }
    
time_out:
    return SearchResult{bestMove, bestScore, finalDepth, nodes_};
}

int Searcher::negamax(Board& board, Stone color, int depth, int alpha, int beta) {
    nodes_++;
    
    // 检查胜负
    Stone winner = board.checkWin();
    if (winner != Stone::Empty) {
        return (winner == rootColor_) ? 999999999 : -999999999;
    }
    
    // 叶子节点
    if (depth <= 0) {
        int score = evaluator_.evaluate(board, rootColor_);
        return (color == rootColor_) ? score : -score;
    }
    
    // 置换表查询
    uint64_t hash = Zobrist::hash(board);
    int ttScore;
    Point ttMove;
    TTEntry::Flag ttFlag;
    
    if (tt_.probe(hash, depth, ttScore, ttMove, ttFlag)) {
        if (ttFlag == TTEntry::EXACT) return ttScore;
        if (ttFlag == TTEntry::ALPHA && ttScore <= alpha) return ttScore;
        if (ttFlag == TTEntry::BETA && ttScore >= beta) return ttScore;
    }
    
    // 生成着法
    auto moves = moveGen_.generateMoves(board, color, 12);
    if (moves.empty()) return 0;
    
    int bestScore = -999999999;
    Point bestMove = moves[0];
    
    for (auto& move : moves) {
        board.place(move, color);
        Stone oppColor = (color == Stone::Black) ? Stone::White : Stone::Black;
        int score = -negamax(board, oppColor, depth - 1, -beta, -alpha);
        board.undo();
        
        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
        }
        
        alpha = std::max(alpha, score);
        if (alpha >= beta) {
            // Beta 剪枝
            tt_.store(hash, depth, bestScore, bestMove, TTEntry::BETA);
            return bestScore;
        }
    }
    
    // 存储到置换表
    TTEntry::Flag flag = (bestScore <= alpha) ? TTEntry::ALPHA : TTEntry::EXACT;
    tt_.store(hash, depth, bestScore, bestMove, flag);
    
    return bestScore;
}
