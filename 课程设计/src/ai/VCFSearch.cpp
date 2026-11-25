#include "VCFSearch.h"
#include "VCFWinningPoints.h"
#include "Pattern.h"
#include <algorithm>

VCFSearcher::VCFSearcher(Evaluator& evaluator):evaluator_(evaluator) {}

bool VCFSearcher::interrupted() {
    return (cancel_ && cancel_->load()) || std::chrono::steady_clock::now()>=deadline_;
}

bool VCFSearcher::stopped() {
    if(aborted_) return true;
    if(result_.workUnits>=options_.maxWorkUnits || interrupted()) {
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

bool VCFSearcher::winningPoints(const Board& board,const vcf_detail::WinningPoints& points,
                               Stone color,std::vector<Point>& wins) {
    wins.clear();
    const auto mask=points.find(color);
    long long count=0;
    board.forEachCandidate([&](Point p) {
        ++count;
        if(mask.test(p.y*BoardConst::SIZE+p.x)) wins.push_back(p);
    });
    // 批量计入原查询逐格消耗的工作量，保证相同额度仍检查相同的进攻分支。
    const auto allowed=std::min<long long>(options_.maxWorkUnits-result_.workUnits,count);
    result_.workUnits+=allowed;
    if(allowed!=count || interrupted()) {
        aborted_=true;
        return false;
    }
    return true;
}

bool VCFSearcher::prove(Board& board,vcf_detail::WinningPoints& points,Stone attacker,
                        int remaining,Point rootMove,int attackPly,Point& provenMove,int& provenPly) {
    if(!tick()) return false;
    ++result_.nodes;

    if(attackPly>=options_.maxAttackMoves) return false;
    std::vector<Point> immediateWins;
    if(!winningPoints(board,points,attacker,immediateWins)) return false;
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
        points.update(attack,attacker,true);

        bool proof=false;
        int nextPly=attackPly+1;
        currentLine_.push_back(attack);
        if(board.checkWin()==attacker) {
            proof=true;
            result_.attackLine=currentLine_;
        } else {
            std::vector<Point> defenderWins, attackerWins;
            if(winningPoints(board,points,defender,defenderWins) &&
               winningPoints(board,points,attacker,attackerWins) &&
               defenderWins.empty() && !attackerWins.empty()) {
                if(attackerWins.size()>=2) {
                    proof=true;
                    result_.attackLine=currentLine_;
                } else if(remaining>1) {
                    // 唯一成五点必须挡住；多个成五点无法用一手全部挡住。
                    const Point block=attackerWins.front();
                    if(board.place(block,defender)) {
                        points.update(block,defender,true);
                        if(board.checkWin()!=defender)
                            proof=prove(board,points,attacker,remaining-1,
                                        rootMove.isValid()?rootMove:attack,
                                        attackPly+1,provenMove,provenPly);
                        points.update(block,defender,false);
                        board.undo();
                    }
                }
            }
        }
        points.update(attack,attacker,false);
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
    vcf_detail::WinningPoints points(board);
    Point move;
    int attackPly=0;
    if(prove(board,points,attacker,options_.maxAttackMoves,Point{},0,move,attackPly)) {
        if(interrupted()) {
            result_.status=VCFStatus::Aborted;
            result_.attackLine.clear();
            return result_;
        }
        result_.status=VCFStatus::Proven;
        result_.move=move;
        result_.attackMoves=attackPly;
    } else if(aborted_) result_.status=VCFStatus::Aborted;
    return result_;
}
