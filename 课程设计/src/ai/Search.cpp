#include "Search.h"
#include <algorithm>
#include <cstdlib>

namespace { constexpr int INF=SearchScore::Infinity; }
Searcher::Searcher(Evaluator& e,MoveGenerator& m):evaluator_(e),moveGen_(m),tt_(64) {}
bool Searcher::expired() {
    if((cancel_ && cancel_->load()) || std::chrono::steady_clock::now()>=deadline_) timeUp_=true;
    return timeUp_;
}
void Searcher::orderMoves(const Board& board,std::vector<Point>& moves,Stone color,int ply,Point ttMove) {
    if(!options_.heuristics || moves.size()<2) return;
    std::array<int,225> tiers{};
    std::array<int,225> scores{};
    const int own=color==Stone::Black?0:1;
    for(Point p:moves) {
        const auto pair=evaluator_.evaluateMovePair(board,p);
        const int attack=pair[own], defense=pair[1-own];
        int tier=attack>=ShapeScore::FIVE ? 0 : defense>=ShapeScore::FIVE ? 1 :
                 defense>=ShapeScore::BLOCKED_FOUR ? 2 : attack>=ShapeScore::BLOCKED_FOUR ? 3 : 6;
        // Quiet TT/killer moves are useful, but never outrank direct wins,
        // immediate blocks, or four-making/ four-blocking tactical tiers.
        if(tier==6 && p==ttMove) tier=4;
        else if(tier==6 && ply<64 && (p==killers_[ply][0] || p==killers_[ply][1])) tier=5;
        const int id=p.y*15+p.x;
        tiers[id]=tier;
        scores[id]=attack+defense*11/10;
    }
    std::stable_sort(moves.begin(),moves.end(),[&](Point a,Point b) {
        const int ia=a.y*15+a.x, ib=b.y*15+b.x;
        if(tiers[ia]!=tiers[ib]) return tiers[ia]<tiers[ib];
        if(tiers[ia]<=3 && scores[ia]!=scores[ib]) return scores[ia]>scores[ib];
        if(tiers[ia]==6) {
            const int ha=history_[own][ia], hb=history_[own][ib];
            if(ha!=hb) return ha>hb;
        }
        return false;
    });
    const int firstId=moves.front().y*15+moves.front().x;
    if(tiers[firstId]<=3) ++stats_.tacticalPriorityFirst;
    if(ply<64 && tiers[firstId]==5) ++stats_.killerHits;
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
    options_.quiescenceDepth=std::clamp(options_.quiescenceDepth,0,8);
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
        // A proven win cannot improve beyond the shortest winning line. A proven loss
        // must keep searching: deeper iterations may find the longest resistance.
        if(score>=SearchScore::MateThreshold) break;
    }
    result.nodes=nodes_+stats_.quiescenceNodes;
    result.quiescenceNodes=stats_.quiescenceNodes;
    if(cancel_ && cancel_->load()) { result.move=Point{}; result.status=SearchStatus::Cancelled; }
    return result;
}
int Searcher::quiesce(Board& board,Stone color,int alpha,int beta,int ply,int remaining) {
    ++stats_.quiescenceNodes;
    if(expired()) return 0;

    const Stone winner=board.checkWin();
    if(winner!=Stone::Empty)
        return winner==color ? SearchScore::Mate-ply : -SearchScore::Mate+ply;
    if(board.getHistory().size()==225) return 0;

    // Score both sides' candidate points together. The move cache makes this second
    // pass cheap and, importantly, the winning points are handled before stand-pat.
    const Stone opponent=color==Stone::Black ? Stone::White : Stone::Black;
    const int ownIndex=color==Stone::Black ? 0 : 1;
    std::vector<Point> candidates=moveGen_.generateMoves(board,color,225);
    if(expired()) return 0;
    if(candidates.empty())
        return std::clamp(evaluator_.evaluate(board,color),-SearchScore::Mate/2,SearchScore::Mate/2);

    std::vector<Point> ownWins;
    std::vector<Point> opponentWins;
    std::vector<ScoredMove> forcing;
    ownWins.reserve(4); opponentWins.reserve(4); forcing.reserve(12);
    for(Point move:candidates) {
        const auto scores=evaluator_.evaluateMovePair(board,move);
        const int attack=scores[ownIndex];
        const int defense=scores[1-ownIndex];
        if(attack>=ShapeScore::FIVE) ownWins.push_back(move);
        if(defense>=ShapeScore::FIVE) opponentWins.push_back(move);
        else if(attack>=ShapeScore::BLOCKED_FOUR || defense>=ShapeScore::BLOCKED_FOUR)
            forcing.push_back({move,attack+defense*11/10});
    }
    if(!ownWins.empty()) return SearchScore::Mate-(ply+1);
    if(opponentWins.size()>1) return -SearchScore::Mate+(ply+2);

    // A unique immediate win must be answered; stand-pat would represent an
    // illegal pass here. At the extension boundary, make the forced block and
    // evaluate the resulting position rather than growing without bound.
    if(opponentWins.size()==1) {
        const Point block=opponentWins.front();
        if(expired() || !board.place(block,color)) return 0;
        evaluator_.makeMove(board,block);
        ++stats_.forcedBlocks;
        int value;
        if(remaining>0 && ply<63) value=-quiesce(board,opponent,-beta,-alpha,ply+1,remaining-1);
        else value=-std::clamp(evaluator_.evaluate(board,opponent),-SearchScore::Mate/2,SearchScore::Mate/2);
        board.undo(); evaluator_.unmakeMove();
        return value;
    }

    const int standPat=std::clamp(evaluator_.evaluate(board,color),-SearchScore::Mate/2,SearchScore::Mate/2);
    if(remaining<=0 || ply>=63 || forcing.empty()) return standPat;
    if(standPat>=beta) return standPat;
    if(standPat>alpha) alpha=standPat;
    std::stable_sort(forcing.begin(),forcing.end());

    int best=standPat;
    for(const ScoredMove& candidate:forcing) {
        if(expired()) return 0;
        const Point move=candidate.move;
        if(!board.place(move,color)) continue;
        evaluator_.makeMove(board,move);
        ++stats_.forcingMoves;
        int score;
        const Stone next=opponent;
        if(board.checkWin()==color) score=SearchScore::Mate-(ply+1);
        else score=-quiesce(board,next,-beta,-alpha,ply+1,remaining-1);
        board.undo(); evaluator_.unmakeMove();
        if(timeUp_) return 0;
        if(score>best) best=score;
        if(score>alpha) alpha=score;
        if(alpha>=beta) break;
    }
    return best;
}

int Searcher::negamax(Board& board,Stone color,int depth,int alpha,int beta,int ply) {
    if(expired()) return 0;
    Stone winner=board.checkWin();
    if(winner!=Stone::Empty) { ++nodes_; return winner==color ? SearchScore::Mate-ply : -SearchScore::Mate+ply; }
    if(board.getHistory().size()==225) { ++nodes_; return 0; }
    if(depth<=0) {
        if(options_.quiescence) return quiesce(board,color,alpha,beta,ply,options_.quiescenceDepth);
        ++nodes_;
        return std::clamp(evaluator_.evaluate(board,color),-SearchScore::Mate/2,SearchScore::Mate/2);
    }
    ++nodes_;
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
    orderMoves(board,moves,color,ply,ttMove);
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
