#include "ai/AIEngine.h"
#include "ai/VCFSearch.h"
#include <atomic>
#include <iostream>
#include <random>
#include <stdexcept>

void require(bool value,const char* reason) { if(!value) throw std::runtime_error(reason); }
void place(Board& board,Point point,Stone color) {
    require(board.place(point,color),"fixture setup failed");
}
void checkProof(const Board& board,const VCFResult& result,Stone attacker) {
    require(result.status==VCFStatus::Proven && result.move.isValid(),"expected VCF proof");
    require(!result.attackLine.empty() && result.attackLine.front()==result.move,"proof line missing root move");
    Board after=board;
    const Stone defender=attacker==Stone::Black?Stone::White:Stone::Black;
    for(Point attack:result.attackLine) {
        require(after.place(attack,attacker),"proof line attack must be legal");
        if(after.checkWin()==attacker) return;
        Evaluator evaluator; std::vector<Point> wins; after.appendCandidates(wins);
        std::vector<Point> winning;
        for(Point p:wins) if(evaluator.evaluateMove(after,p,attacker)>=ShapeScore::FIVE) {
            if(after.place(p,attacker)) { if(after.checkWin()==attacker) winning.push_back(p); after.undo(); }
        }
        require(!winning.empty(),"every nonterminal proof attack must create a winning point");
        if(winning.size()>=2) return;
        require(after.place(winning.front(),defender),"forced defense must be legal");
        require(after.checkWin()!=defender,"defender cannot win while blocking a proof line");
    }
    throw std::runtime_error("proof line ended without a win or terminal fork");
}
int main() {
    Board fork;
    for(int x=4;x<=6;++x) place(fork,Point(x,7),Stone::Black);
    for(int y=4;y<=6;++y) place(fork,Point(7,y),Stone::Black);
    place(fork,Point(3,7),Stone::White);
    place(fork,Point(7,3),Stone::White);
    Evaluator eval; VCFSearcher vcf(eval);
    const uint64_t originalHash=fork.getHash();
    const size_t originalMoves=fork.getHistory().size();
    VCFOptions options; options.maxAttackMoves=4; options.maxWorkUnits=30000; options.timeBudgetMs=500;
    VCFResult proven=vcf.find(fork,Stone::Black,options);
    checkProof(fork,proven,Stone::Black);
    require(fork.getHash()==originalHash && fork.getHistory().size()==originalMoves,"proof search modified source board");
    std::cout<<"Fork proof move="<<proven.move.x<<','<<proven.move.y<<" work="<<proven.workUnits<<"\n";

    Board blockable;
    for(int x=4;x<=6;++x) place(blockable,Point(x,7),Stone::Black);
    place(blockable,Point(3,7),Stone::White);
    VCFResult shallow=vcf.find(blockable,Stone::Black,options);
    require(shallow.status==VCFStatus::NotProven,"a single blockable four must not be reported as proof");

    Board exhausted;
    place(exhausted,Point(7,7),Stone::Black);
    place(exhausted,Point(8,8),Stone::White);
    VCFResult blocked=vcf.find(exhausted,Stone::Black,options);
    require(blocked.status==VCFStatus::NotProven,"a quiet position must not prove VCF");

    Board counterWin;
    for(int x=4;x<=6;++x) place(counterWin,Point(x,7),Stone::Black);
    for(int y=4;y<=6;++y) place(counterWin,Point(7,y),Stone::Black);
    for(int x=4;x<=7;++x) place(counterWin,Point(x,10),Stone::White);
    VCFResult counter=vcf.find(counterWin,Stone::Black,options);
    require(counter.status==VCFStatus::NotProven,"defender's immediate counter-win must refute attack");

    std::atomic<bool> cancelled{true};
    VCFResult cancel=vcf.find(fork,Stone::Black,options,&cancelled);
    require(cancel.status==VCFStatus::Aborted && !cancel.move.isValid(),"cancel must never report an unproved move");
    VCFOptions tiny=options; tiny.maxWorkUnits=1;
    VCFResult bounded=vcf.find(fork,Stone::Black,tiny);
    require(bounded.status==VCFStatus::Aborted && !bounded.move.isValid() && bounded.workUnits<=1,
            "work-budget exhaustion must safely abort");

    SearchOptions aiOptions; aiOptions.vcf=true; aiOptions.vcfTimeBudgetMs=250;
    AIEngine engine;
    SearchResult integrated=engine.think(fork,1200,nullptr,aiOptions);
    require(integrated.status==SearchStatus::Move && integrated.reason==SearchReason::VCFProof,
            "AIEngine must use opt-in proven VCF result");
    require(integrated.move==proven.move && integrated.vcfNodes>0 && integrated.vcfWorkUnits>0,
            "integrated VCF result/diagnostics mismatch");
    AIEngine baseline;
    SearchOptions disabled=aiOptions; disabled.vcf=false;
    SearchResult ordinary=baseline.think(fork,700,nullptr,disabled);
    require(ordinary.status==SearchStatus::Move && ordinary.move.isValid() && ordinary.reason!=SearchReason::VCFProof,
            "disabled VCF must retain legal standard-search fallback");

    std::mt19937 rng(0x5eed);
    for(int game=0;game<24;++game) {
        Board random;
        for(int ply=0;ply<18 && random.checkWin()==Stone::Empty;++ply) {
            Point p(static_cast<int>(rng()%15),static_cast<int>(rng()%15));
            if(random.place(p,random.getCurrentPlayer()) && random.checkWin()!=Stone::Empty) break;
        }
        VCFResult randomResult=vcf.find(random,random.getCurrentPlayer(),options);
        if(randomResult.status==VCFStatus::Proven) checkProof(random,randomResult,random.getCurrentPlayer());
    }
    std::cout<<"VCF proof, counter-threat, cancellation, budget and integration regressions passed\n";
}
