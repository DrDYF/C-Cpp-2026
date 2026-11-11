#include "VCFSearch.h"
#include "Pattern.h"
#include <algorithm>

VCFSearcher::VCFSearcher(Evaluator& evaluator):evaluator_(evaluator) {}

bool VCFSearcher::stopped() {
    if(aborted_) return true;
    if((cancel_ && cancel_->load()) || result_.workUnits>=options_.maxWorkUnits ||
       std::chrono::steady_clock::now()>=deadline_) {
        aborted_=true;
        return true;
    }
    return false;
}

bool VCFSearcher::tick() {
    if(stopped()) return false;
    ++result_.workUnits;
    return true;
}

bool VCFSearcher::winningPoints(Board& board,Stone color,std::vector<Point>& wins) {
    wins.clear();
    std::vector<Point> candidates;
    board.appendCandidates(candidates);
    const int side=color==Stone::Black?0:1;
    for(Point p:candidates) {
        if(!tick()) return false;
        const auto scores=evaluator_.evaluateMovePair(board,p);
        if(scores[side]<ShapeScore::FIVE) continue;
        if(!board.place(p,color)) continue;
        const bool won=board.checkWin()==color;
        board.undo();
        if(won) wins.push_back(p);
    }
    return true;
}

bool VCFSearcher::prove(Board& board,Stone attacker,int remaining,Point rootMove,
                        int attackPly,Point& provenMove,int& provenPly) {
    if(!tick()) return false;
    ++result_.nodes;

    if(attackPly>=options_.maxAttackMoves) return false;
    std::vector<Point> immediateWins;
    if(!winningPoints(board,attacker,immediateWins)) return false;
    if(!immediateWins.empty()) {
        provenMove=rootMove.isValid()?rootMove:immediateWins.front();
        provenPly=attackPly+1;
        result_.attackLine=currentLine_;
        result_.attackLine.push_back(immediateWins.front());
        return true;
    }
    if(remaining<=0) return false;

    const Stone defender=attacker==Stone::Black?Stone::White:Stone::Black;
    std::vector<Point> candidates;
    board.appendCandidates(candidates);
    const int attackIndex=attacker==Stone::Black?0:1;
    for(Point attack:candidates) {
        if(!tick()) return false;
        if(evaluator_.evaluateMovePair(board,attack)[attackIndex]<ShapeScore::BLOCKED_FOUR) continue;
        if(!board.place(attack,attacker)) continue;

        bool proof=false;
        int nextPly=attackPly+1;
        currentLine_.push_back(attack);
        if(board.checkWin()==attacker) {
            proof=true;
            result_.attackLine=currentLine_;
        } else {
            std::vector<Point> defenderWins, attackerWins;
            if(!winningPoints(board,defender,defenderWins) || !winningPoints(board,attacker,attackerWins)) {
                board.undo();
                currentLine_.pop_back();
                return false;
            }
            // A defender's immediate counter-win always refutes this branch.
            if(defenderWins.empty() && !attackerWins.empty()) {
                if(attackerWins.size()>=2) {
                    proof=true;
                    result_.attackLine=currentLine_;
                } else if(remaining>1) {
                    // With exactly one winning square there is only one defense
                    // that avoids an immediate loss: occupy that square.
                    const Point block=attackerWins.front();
                    if(board.place(block,defender)) {
                        if(board.checkWin()!=defender)
                            proof=prove(board,attacker,remaining-1,
                                        rootMove.isValid()?rootMove:attack,
                                        attackPly+1,provenMove,provenPly);
                        board.undo();
                    }
                }
            }
        }
        board.undo();
        currentLine_.pop_back();
        if(aborted_) return false;
        if(proof) {
            if(!provenMove.isValid()) provenMove=rootMove.isValid()?rootMove:attack;
            if(provenPly==0) provenPly=nextPly;
            return true;
        }
    }
    return false;
}

VCFResult VCFSearcher::find(const Board& source,Stone attacker,VCFOptions options,
                            const std::atomic<bool>* cancel) {
    result_=VCFResult{};
    currentLine_.clear();
    options_=options;
    options_.maxAttackMoves=std::clamp(options_.maxAttackMoves,1,12);
    options_.maxWorkUnits=std::clamp(options_.maxWorkUnits,1,1000000);
    options_.timeBudgetMs=std::clamp(options_.timeBudgetMs,1,5000);
    cancel_=cancel;
    aborted_=false;
    deadline_=std::chrono::steady_clock::now()+std::chrono::milliseconds(options_.timeBudgetMs);
    if(attacker==Stone::Empty || source.checkWin()!=Stone::Empty || stopped()) {
        if(aborted_) result_.status=VCFStatus::Aborted;
        return result_;
    }
    Board board=source;
    Point move;
    int attackPly=0;
    if(prove(board,attacker,options_.maxAttackMoves,Point{},0,move,attackPly)) {
        result_.status=VCFStatus::Proven;
        result_.move=move;
        result_.attackMoves=attackPly;
    } else if(aborted_) result_.status=VCFStatus::Aborted;
    return result_;
}
