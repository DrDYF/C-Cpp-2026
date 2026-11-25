#include "AIEngine.h"
#include "VCFWinningPoints.h"
#include <algorithm>
#include <chrono>

namespace {
constexpr int Side=BoardConst::SIZE;
constexpr int Cells=Side*Side;

int firstPoint(const std::bitset<Cells>& mask) {
    for(int i=0;i<Cells;++i)
        if(mask.test(i)) return i;
    return -1;
}
}

AIEngine::AIEngine()
    : moveGen_(evaluator_), searcher_(evaluator_, moveGen_), vcfSearcher_(evaluator_) {}

SearchResult AIEngine::think(const Board& board,int timeLimitMs,const std::atomic<bool>* cancel,SearchOptions options) {
    const auto start=std::chrono::steady_clock::now();
    auto cancelled=[] { SearchResult r; r.status=SearchStatus::Cancelled; return r; };
    if(cancel && cancel->load()) return cancelled();
    const auto& history=board.getHistory();
    if(board.checkWin()!=Stone::Empty || history.size()==Cells) {
        SearchResult r; r.status=SearchStatus::Terminal; return r;
    }
    if(history.empty()) return {Point(7,7),0,1,1,SearchReason::Opening,SearchStatus::Move};
    const Stone current=board.getCurrentPlayer();
    const Stone opponent=current==Stone::Black?Stone::White:Stone::Black;
    const vcf_detail::WinningPoints points(board);
    const int ownWin=firstPoint(points.find(current));
    if(cancel && cancel->load()) return cancelled();
    if(ownWin>=0) {
        // 保留原按行试落时访问的空点数，与既有统计口径一致。
        long long tacticalNodes=ownWin+1;
        for(Point p:history)
            if(p.y*Side+p.x<ownWin) --tacticalNodes;
        return {Point(ownWin%Side,ownWin/Side),SearchScore::Mate-1,1,tacticalNodes,
                SearchReason::ImmediateWin,SearchStatus::Move};
    }
    const auto opponentWins=points.find(opponent);
    if(cancel && cancel->load()) return cancelled();
    const auto threatCount=opponentWins.count();
    const long long tacticalNodes=2*(Cells-static_cast<long long>(history.size()));
    if(threatCount==1) {
        const int blockId=firstPoint(opponentWins);
        const Point block(blockId%Side,blockId/Side);
        Board probe=board;
        probe.place(block,current);
        int defendedScore=evaluator_.evaluateFull(probe,current);
        if(cancel && cancel->load()) return cancelled();
        return {block,defendedScore,1,tacticalNodes,SearchReason::ForcedDefense,SearchStatus::Move};
    }
    int elapsed=static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count());
    int remaining=std::max(1,timeLimitMs-elapsed);
    if(options.vcf && remaining>0) {
        VCFOptions vcfOptions;
        vcfOptions.maxAttackMoves=options.vcfMaxAttackMoves;
        vcfOptions.maxWorkUnits=options.vcfWorkUnits;
        vcfOptions.timeBudgetMs=std::min(options.vcfTimeBudgetMs,remaining);
        VCFResult vcf=vcfSearcher_.find(board,current,vcfOptions,cancel);
        if(vcf.status==VCFStatus::Proven && vcf.move.isValid()) {
            SearchResult result{vcf.move,SearchScore::Mate-vcf.attackMoves,vcf.attackMoves,
                                tacticalNodes+vcf.nodes,SearchReason::VCFProof,SearchStatus::Move};
            result.vcfNodes=vcf.nodes;
            result.vcfWorkUnits=vcf.workUnits;
            return result;
        }
        if(cancel && cancel->load()) return cancelled();
        elapsed=static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count());
        remaining=std::max(1,timeLimitMs-elapsed);
    }
    SearchResult result=searcher_.search(board,current,remaining,cancel,options);
    result.nodes+=tacticalNodes;
    if(threatCount>1 && result.status==SearchStatus::Move) {
        result.score=-SearchScore::Mate+2; result.reason=SearchReason::MultipleThreatLoss;
    }
    return result;
}
