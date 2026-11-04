#pragma once
#include "core/Board.h"
#include "Evaluator.h"
#include "MoveGen.h"
#include "Search.h"
#include "ThreatSearch.h"

class AIEngine {
public:
    AIEngine();
    
    SearchResult think(const Board& board, int timeLimitMs = 1500);
    
private:
    Evaluator evaluator_;
    MoveGenerator moveGen_;
    Searcher searcher_;
    ThreatSearcher threatSearcher_;
};
