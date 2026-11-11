#pragma once
#include "Evaluator.h"
#include <chrono>
#include <atomic>
#include <vector>

enum class VCFStatus { NotProven, Proven, Aborted };

struct VCFOptions {
    int maxAttackMoves=8;
    int maxWorkUnits=30000;
    int timeBudgetMs=100;
};

struct VCFResult {
    VCFStatus status=VCFStatus::NotProven;
    Point move;
    int attackMoves=0;
    long long nodes=0;
    long long workUnits=0;
    std::vector<Point> attackLine;
};

// Conservative continuous-four proof search. A proof is returned only when
// every legal defense to each single winning point is forced; uncertain or
// bounded-out lines are never treated as wins.
class VCFSearcher {
public:
    explicit VCFSearcher(Evaluator& evaluator);
    VCFResult find(const Board& board,Stone attacker,VCFOptions options=VCFOptions{},
                   const std::atomic<bool>* cancel=nullptr);
private:
    bool prove(Board& board,Stone attacker,int remaining,Point rootMove,int attackPly,Point& provenMove,int& provenPly);
    bool winningPoints(Board& board,Stone color,std::vector<Point>& wins);
    bool tick();
    bool stopped();
    Evaluator& evaluator_;
    VCFOptions options_;
    const std::atomic<bool>* cancel_=nullptr;
    std::chrono::steady_clock::time_point deadline_;
    VCFResult result_;
    bool aborted_=false;
    std::vector<Point> currentLine_;
};
