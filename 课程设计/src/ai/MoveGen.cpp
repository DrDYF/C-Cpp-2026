#include "MoveGen.h"
#include "VCFWinningPoints.h"
#include <algorithm>
#include <cstdlib>

namespace {
int pointId(Point p) { return p.y*BoardConst::SIZE+p.x; }

bool openFourBlocks(const Board& board,Stone opponent,const std::vector<Point>& threats,MoveMask& blocks) {
    if(threats.empty()) return false;
    vcf_detail::WinningPoints wins(board);
    bool found=false;
    for(Point threat:threats) {
        wins.update(threat,opponent,true);
        auto ends=wins.find(opponent);
        wins.update(threat,opponent,false);
        const auto count=ends.count();
        if(count<2) continue;
        // 先占威胁点，或去掉恰有两个胜点中的一个；同时有多处威胁时必须都化解。
        if(count>2) ends.reset();
        ends.set(pointId(threat));
        if(found) blocks&=ends;
        else { blocks=ends; found=true; }
        if(blocks.none()) break;
    }
    return found;
}
}

MoveGenerator::MoveGenerator(Evaluator& evaluator):evaluator_(evaluator) {}

std::vector<Point> MoveGenerator::generateMoves(const Board& board,Stone color,int maxCount,MoveTierMode mode,
                                               std::array<std::array<int,2>,225>* scores,Point* omittedFourThree,
                                               MoveTailPolicy tailPolicy,MoveMask* priority) {
    if(omittedFourThree) *omittedFourThree=Point{};
    if(priority) priority->reset();
    if(maxCount<=0 || board.checkWin()!=Stone::Empty) return {};
    std::vector<ScoredMove> scored;
    scored.reserve(std::min(225,maxCount)+16);
    const int own=color==Stone::Black?0:1;
    const bool defend=mode==MoveTierMode::Structural && tailPolicy==MoveTailPolicy::DefendOpenFour;
    bool immediate=false;
    std::vector<Point> openFourThreats;
    evaluator_.forEachCandidateAnalysis(board,[&](Point p,const MoveAnalysis& analysis) {
        if(scores) (*scores)[p.y*15+p.x]={analysis[0].score,analysis[1].score};
        const int attack=analysis[own].score, defense=analysis[1-own].score;
        int tier;
        bool retainFork=false;
        if(mode==MoveTierMode::LegacyScore) {
            tier=attack>=ShapeScore::FIVE ? 0 : defense>=ShapeScore::FIVE ? 1 :
                 defense>=ShapeScore::BLOCKED_FOUR ? 2 : attack>=ShapeScore::BLOCKED_FOUR ? 3 : 4;
        } else {
            tier=analysis[own].five ? 0 : analysis[1-own].five ? 1 :
                 analysis[1-own].fours>0 ? 2 : analysis[own].fours>0 ? 3 : 4;
            // 同方向有两个胜点，或不同方向各有一个胜点。
            retainFork=analysis[own].openFours>0 || analysis[own].fours>1;
            if(defend) {
                immediate|=tier<=1;
                if(analysis[1-own].openFours>0) openFourThreats.push_back(p);
            }
        }
        int score=attack+defense*11/10;
        score=score*4+28-(std::abs(p.x-7)+std::abs(p.y-7));
        scored.push_back({p,score,tier,retainFork});
    });
    const auto less=[](const ScoredMove& a,const ScoredMove& b) {
        if(a<b) return true;
        if(b<a) return false;
        return a.move.y*15+a.move.x<b.move.y*15+b.move.x;
    };
    const auto k=std::min<std::size_t>(maxCount,scored.size());
    auto middle=scored.begin()+k;
    std::partial_sort(scored.begin(),middle,scored.end(),less);
    MoveMask defenseStage;
    if(defend && !immediate && openFourBlocks(board,color==Stone::Black?Stone::White:Stone::Black,
                                   openFourThreats,defenseStage)) {
        // 冲四可以在别处先手反击，但能否救局仍由搜索判定。
        for(const auto& candidate:scored) {
            const auto& attack=evaluator_.readGeneratedAnalysis(board,candidate.move)[own];
            if(attack.fours>0) defenseStage.set(pointId(candidate.move));
        }
        if(priority) *priority=defenseStage;
    }
    const bool staged=defenseStage.any();
    // 冲四迫使对手应手，不能和普通着一起按固定名额丢掉。
    const int retainedTier=mode==MoveTierMode::Structural && tailPolicy==MoveTailPolicy::KeepForcingFours ? 3 : 2;
    auto end=std::partition(middle,scored.end(),[&](const ScoredMove& m) {
        return m.tier<=retainedTier || m.retainFork || (staged && defenseStage.test(pointId(m.move)));
    });
    std::sort(middle,end,less);
    if(omittedFourThree && mode==MoveTierMode::Structural) {
        // 只查看被截掉的冲四，沿用已计算的棋形和排名，不重造候选或排序。
        auto best=scored.end();
        for(auto it=end;it!=scored.end();++it) {
            if(it->tier!=3 || (best!=scored.end() && !less(*it,*best))) continue;
            const auto& attack=evaluator_.readGeneratedAnalysis(board,it->move)[own];
            if(attack.threes>0) best=it;
        }
        if(best!=scored.end()) *omittedFourThree=best->move;
    }
    std::vector<Point> result;
    result.reserve(end-scored.begin());
    if(staged) {
        for(auto it=scored.begin();it!=end;++it)
            if(defenseStage.test(pointId(it->move))) result.push_back(it->move);
    }
    for(auto it=scored.begin();it!=end;++it)
        if(!staged || !defenseStage.test(pointId(it->move))) result.push_back(it->move);
    return result;
}
