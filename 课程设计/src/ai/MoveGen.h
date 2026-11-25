#pragma once
#include "core/Board.h"
#include "Evaluator.h"
#include <bitset>
#include <vector>

struct ScoredMove {
    Point move;
    int score;
    int tier=4;
    bool retainFork=false;
    bool operator<(const ScoredMove& other) const {
        if(tier!=other.tier) return tier<other.tier;
        return score>other.score;
    }
};
enum class MoveTierMode { Structural, LegacyScore };
enum class MoveTailPolicy { Standard, KeepForcingFours, DefendOpenFour };
using MoveMask = std::bitset<BoardConst::SIZE * BoardConst::SIZE>;

class MoveGenerator {
public:
    explicit MoveGenerator(Evaluator& evaluator);
    std::vector<Point> generateMoves(const Board& board,Stone color,int maxCount=16,
                                     MoveTierMode mode=MoveTierMode::Structural,
                                     std::array<std::array<int,2>,225>* scores=nullptr,
                                     Point* omittedFourThree=nullptr,
                                     MoveTailPolicy tailPolicy=MoveTailPolicy::Standard,
                                     MoveMask* priority=nullptr);
private:
    friend class Searcher;
    Evaluator& evaluator_;
};
