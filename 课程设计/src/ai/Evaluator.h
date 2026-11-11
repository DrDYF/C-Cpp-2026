#pragma once
#include "core/Board.h"
#include "Pattern.h"
#include <array>
#include <vector>

struct MoveThreat {
    bool five=false;
    int fours=0, openFours=0, threes=0;
    bool forcing() const { return five || fours>0; }
    bool decisiveThreat() const { return openFours>0 || fours>1 || (fours>0 && threes>0); }
};
struct MoveEvalStats { long long lineScans=0,cacheHits=0; };
class Evaluator {
public:
    Evaluator();
    int evaluate(const Board& board,Stone color);
    int evaluateFull(const Board& board,Stone color) const;
    int evaluateMove(const Board& board,Point p,Stone color);
    std::array<int,2> evaluateMovePair(const Board& board,Point p);
    std::array<MoveThreat,2> moveThreats(const Board& board,Point p);
    int evaluateMoveReference(const Board& board,Point p,Stone color) const;
    MoveEvalStats moveStats() const { return moveStats_; }
    void resetMoveStats() { moveStats_={}; }
    void beginPosition(const Board& board);
    void makeMove(const Board& board,Point p);
    void unmakeMove();
private:
    int lineScore(const Board& board,int id,Stone color) const;
    int evaluateColor(const Board& board,Stone color) const;
    void syncMoveCache(const Board& board);
    void invalidateMoveCache(Point p);
    struct Undo {
        std::array<int,4> ids;
        std::array<std::array<int,2>,4> scores;
        std::array<int,2> totals;
        uint64_t hash;
        Point move;
    };
    std::array<std::array<int,2>,88> scores_{};
    std::array<int,2> totals_{};
    std::vector<Undo> undo_;
    uint64_t hash_=0;
    bool valid_=false;
    std::array<std::array<std::array<ShapeType,2>,4>,225> moveShapes_{};
    std::array<uint8_t,225> moveValid_{};
    uint64_t moveHash_=0;
    bool moveCacheValid_=false;
    MoveEvalStats moveStats_;
    PatternRecognizer pattern_;
};
