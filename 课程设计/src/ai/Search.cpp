#include "Search.h"
#include <algorithm>
#include <cstdlib>

namespace { constexpr int INF=SearchScore::Infinity; }
Searcher::Searcher(Evaluator& e,MoveGenerator& m):evaluator_(e),moveGen_(m),tt_(64) {}
bool Searcher::expired() {
    if((cancel_ && cancel_->load()) || std::chrono::steady_clock::now()>=deadline_) timeUp_=true;
    return timeUp_;
}
void Searcher::orderMoves(std::vector<Point>& moves,Stone color,int ply,Point ttMove) {
    if(!options_.heuristics) return;
    auto rank=[&](Point p) {
        if(p==ttMove) return 200000000;
        if(ply<64 && p==killers_[ply][0]) return 100000000;
        if(ply<64 && p==killers_[ply][1]) return 90000000;
        return history_[color==Stone::Black?0:1][p.y*15+p.x];
    };
    std::stable_sort(moves.begin(),moves.end(),[&](Point a,Point b){ return rank(a)>rank(b); });
    if(ply<64 && !moves.empty() && (moves[0]==killers_[ply][0] || moves[0]==killers_[ply][1])) ++stats_.killerHits;
}
void Searcher::recordCutoff(Point move,Stone color,int depth,int ply) {
    if(!options_.heuristics) return;
    if(ply<64 && killers_[ply][0]!=move) { killers_[ply][1]=killers_[ply][0]; killers_[ply][0]=move; }
    int& value=history_[color==Stone::Black?0:1][move.y*15+move.x];
    value=std::min(1000000,value+depth*depth);
    ++stats_.historyUpdates;
}
int Searcher::childScore(Board& board,Stone color,int depth,int alpha,int beta,int ply,bool first) {
    if(first || !options_.pvs) return -negamax(board,color,depth,-beta,-alpha,ply);
    ++stats_.nullWindowProbes;
    int score=-negamax(board,color,depth,-alpha-1,-alpha,ply);
    if(!timeUp_ && score>alpha && score<beta) {
        ++stats_.researches;
        score=-negamax(board,color,depth,-beta,-alpha,ply);
    }
    return score;
}
SearchResult Searcher::search(const Board& board,Stone color,int timeLimitMs,const std::atomic<bool>* cancel,SearchOptions options) {
    cancel_=cancel; nodes_=0; timeUp_=false; options_=options; stats_=SearchStats{};
    killers_={}; history_={};
    deadline_=std::chrono::steady_clock::now()+std::chrono::milliseconds(std::max(1,timeLimitMs));
    SearchResult result;
    if(board.checkWin()!=Stone::Empty || board.getHistory().size()==225) { result.status=SearchStatus::Terminal; return result; }
    if(cancel_ && cancel_->load()) { result.status=SearchStatus::Cancelled; return result; }
    auto candidates=moveGen_.generateMoves(board,color,options_.rootCandidates);
    if(candidates.empty()) {
        for(int y=0;y<15 && candidates.empty();++y) for(int x=0;x<15;++x)
            if(board.at(Point(x,y))==Stone::Empty) { candidates.push_back(Point(x,y)); break; }
        result.reason=SearchReason::Fallback;
    }
    if(candidates.empty()) { result.status=SearchStatus::NoLegalMove; return result; }
    result.move=candidates.front(); result.status=SearchStatus::Move;
    evaluator_.beginPosition(board);
    for(int depth=1;depth<=std::clamp(options_.maxDepth,1,60) && !expired();++depth) {
        if(options_.rootOrdering) {
            auto previous=std::find(candidates.begin(),candidates.end(),result.move);
            if(previous!=candidates.end()) std::rotate(candidates.begin(),previous,previous+1);
        }
        Board position=board;
        int alpha=-INF,beta=INF,score=-INF;
        Point best=candidates.front(); bool first=true;
        for(Point move:candidates) {
            if(expired()) break;
            if(!position.place(move,color)) continue;
            evaluator_.makeMove(position,move);
            Stone opponent=color==Stone::Black?Stone::White:Stone::Black;
            int value=childScore(position,opponent,depth-1,alpha,beta,1,first);
            position.undo(); evaluator_.unmakeMove();
            if(timeUp_) break;
            first=false;
            if(value>score) { score=value; best=move; }
            alpha=std::max(alpha,value);
        }
        if(timeUp_) break;
        result.move=best; result.score=score; result.depth=depth;
        if(std::abs(score)>=SearchScore::MateThreshold) break;
    }
    result.nodes=nodes_;
    if(cancel_ && cancel_->load()) { result.move=Point{}; result.status=SearchStatus::Cancelled; }
    return result;
}
int Searcher::negamax(Board& board,Stone color,int depth,int alpha,int beta,int ply) {
    ++nodes_;
    if(expired()) return 0;
    Stone winner=board.checkWin();
    if(winner!=Stone::Empty) return winner==color ? SearchScore::Mate-ply : -SearchScore::Mate+ply;
    if(board.getHistory().size()==225) return 0;
    if(depth<=0) return std::clamp(evaluator_.evaluate(board,color),-SearchScore::Mate/2,SearchScore::Mate/2);
    uint64_t hash=board.getHash()^(color==Stone::White ? 0x9e3779b97f4a7c15ULL : 0ULL);
    int ttScore; Point ttMove; TTEntry::Flag ttFlag;
    const int originalAlpha=alpha;
    if(tt_.probe(hash,depth,ttScore,ttMove,ttFlag,ply)) {
        if(ttFlag==TTEntry::EXACT) return ttScore;
        if(ttFlag==TTEntry::ALPHA && ttScore<=alpha) return ttScore;
        if(ttFlag==TTEntry::BETA && ttScore>=beta) return ttScore;
    }
    auto moves=moveGen_.generateMoves(board,color,12);
    if(expired()) return 0;
    if(moves.empty()) return 0;
    tt_.probe(hash,0,ttScore,ttMove,ttFlag,ply);
    orderMoves(moves,color,ply,ttMove);
    int bestScore=-INF; Point bestMove=moves.front(); bool first=true;
    for(Point move:moves) {
        if(expired()) return 0;
        if(!board.place(move,color)) continue;
        evaluator_.makeMove(board,move);
        Stone opponent=color==Stone::Black?Stone::White:Stone::Black;
        int score=childScore(board,opponent,depth-1,alpha,beta,ply+1,first);
        board.undo(); evaluator_.unmakeMove();
        if(timeUp_) return 0;
        first=false;
        if(score>bestScore) { bestScore=score; bestMove=move; }
        alpha=std::max(alpha,score);
        if(alpha>=beta) { recordCutoff(move,color,depth,ply); break; }
    }
    TTEntry::Flag flag=bestScore<=originalAlpha ? TTEntry::ALPHA : bestScore>=beta ? TTEntry::BETA : TTEntry::EXACT;
    tt_.store(hash,depth,bestScore,bestMove,flag,ply);
    return bestScore;
}
