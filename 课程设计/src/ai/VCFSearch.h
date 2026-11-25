#pragma once
#include "Evaluator.h"
#include <chrono>
#include <atomic>
#include <vector>

namespace vcf_detail { class WinningPoints; }
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

// 只有所有合法防守都无法破解的连续冲四才返回胜证。
class VCFSearcher {
public:
    explicit VCFSearcher(Evaluator& evaluator);
    VCFResult find(const Board& board,Stone attacker,VCFOptions options=VCFOptions{},
                   const std::atomic<bool>* cancel=nullptr);
private:
    bool prove(Board& board,vcf_detail::WinningPoints& points,Stone attacker,int remaining,
               Point rootMove,int attackPly,Point& provenMove,int& provenPly);
    bool winningPoints(const Board& board,const vcf_detail::WinningPoints& points,Stone color,
                       std::vector<Point>& wins);
    bool tick();
    bool stopped();
    bool interrupted();
    Evaluator& evaluator_;
    VCFOptions options_;
    const std::atomic<bool>* cancel_=nullptr;
    std::chrono::steady_clock::time_point deadline_;
    VCFResult result_;
    bool aborted_=false;
    std::vector<Point> currentLine_;
};
