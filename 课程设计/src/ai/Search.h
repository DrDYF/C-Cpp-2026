#pragma once
#include "core/Board.h"
#include "Evaluator.h"
#include "MoveGen.h"
#include "TranspositionTable.h"
#include <chrono>
#include <atomic>
#include <array>

enum class SearchReason { Normal, Opening, ImmediateWin, ForcedDefense, MultipleThreatLoss, Fallback };
enum class SearchStatus { Unavailable, Move, Terminal, Cancelled, NoLegalMove, Error };
struct SearchResult {
    Point move;
    int score=0;
    int depth=0;
    long long nodes=0;
    SearchReason reason=SearchReason::Normal;
    SearchStatus status=SearchStatus::Unavailable;
};
struct SearchOptions {
    int maxDepth=10;
    bool pvs=true;
    bool heuristics=true;
    bool rootOrdering=true;
    int rootCandidates=16;
};
struct SearchStats {
    long long nullWindowProbes=0, researches=0, killerHits=0, historyUpdates=0;
};
class Searcher {
public:
    Searcher(Evaluator& evaluator,MoveGenerator& moveGen);
    SearchResult search(const Board& board,Stone color,int timeLimitMs,const std::atomic<bool>* cancel=nullptr,
                        SearchOptions options=SearchOptions{});
    const SearchStats& stats() const { return stats_; }
private:
    int negamax(Board& board,Stone color,int depth,int alpha,int beta,int ply);
    int childScore(Board& board,Stone color,int depth,int alpha,int beta,int ply,bool first);
    void orderMoves(std::vector<Point>& moves,Stone color,int ply,Point ttMove);
    void recordCutoff(Point move,Stone color,int depth,int ply);
    bool expired();
    Evaluator& evaluator_;
    MoveGenerator& moveGen_;
    TranspositionTable tt_;
    long long nodes_=0;
    std::chrono::steady_clock::time_point deadline_;
    bool timeUp_=false;
    const std::atomic<bool>* cancel_=nullptr;
    SearchOptions options_;
    SearchStats stats_;
    std::array<std::array<Point,2>,64> killers_{};
    std::array<std::array<int,225>,2> history_{};
};
