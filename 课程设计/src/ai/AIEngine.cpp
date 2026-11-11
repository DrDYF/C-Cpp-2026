#include "AIEngine.h"
#include <algorithm>
#include <chrono>
#include <vector>

AIEngine::AIEngine()
    : moveGen_(evaluator_), searcher_(evaluator_, moveGen_), vcfSearcher_(evaluator_) {}

SearchResult AIEngine::think(const Board& board,int timeLimitMs,const std::atomic<bool>* cancel,SearchOptions options) {
    const auto start=std::chrono::steady_clock::now();
    auto cancelled=[] { SearchResult r; r.status=SearchStatus::Cancelled; return r; };
    if(cancel && cancel->load()) return cancelled();
    if(board.checkWin()!=Stone::Empty || board.getHistory().size()==225) {
        SearchResult r; r.status=SearchStatus::Terminal; return r;
    }
    Stone current=board.getCurrentPlayer();
    if(board.getHistory().empty()) return {Point(7,7),0,1,1,SearchReason::Opening,SearchStatus::Move};
    long long tacticalNodes=0;
    Board probe=board;
    Stone opponent=current==Stone::Black?Stone::White:Stone::Black;
    std::vector<Point> opponentWins;
    for(Stone color:{current,opponent}) {
        for(int y=0;y<15;++y) for(int x=0;x<15;++x) {
            if(cancel && cancel->load()) return cancelled();
            Point p(x,y);
            if(!probe.place(p,color)) continue;
            ++tacticalNodes;
            bool wins=probe.checkWin()==color;
            probe.undo();
            if(!wins) continue;
            if(color==current) return {p,SearchScore::Mate-1,1,tacticalNodes,SearchReason::ImmediateWin,SearchStatus::Move};
            opponentWins.push_back(p);
        }
    }
    if(opponentWins.size()==1) {
        Point block=opponentWins.front();
        probe.place(block,current);
        int defendedScore=evaluator_.evaluateFull(probe,current);
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
    if(opponentWins.size()>1 && result.status==SearchStatus::Move) {
        result.score=-SearchScore::Mate+2; result.reason=SearchReason::MultipleThreatLoss;
    }
    return result;
}
