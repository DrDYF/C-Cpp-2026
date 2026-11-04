#pragma once
#include "core/Board.h"
#include "Evaluator.h"
#include "MoveGen.h"
#include "TranspositionTable.h"

struct SearchResult {
    Point move;
    int score;
    int depth;
    long long nodes;
};

class Searcher {
public:
    Searcher(Evaluator& evaluator, MoveGenerator& moveGen);
    
    SearchResult search(const Board& board, Stone color, int timeLimitMs);
    
private:
    int negamax(Board& board, Stone color, int depth, int alpha, int beta);
    
    Evaluator& evaluator_;
    MoveGenerator& moveGen_;
    TranspositionTable tt_;
    
    long long nodes_;
    Stone rootColor_;
};
