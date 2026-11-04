#pragma once
#include "core/Board.h"
#include "Evaluator.h"
#include "MoveGen.h"
#include "Search.h"

class AIEngine {
public:
    AIEngine();
    SearchResult think(const Board& board,int timeLimitMs=1500,const std::atomic<bool>* cancel=nullptr);
private:
    Evaluator evaluator_;
    MoveGenerator moveGen_;
    Searcher searcher_;
};
