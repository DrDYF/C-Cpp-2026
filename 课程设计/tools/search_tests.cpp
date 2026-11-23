#include "ai/Search.h"
#include "ai/QuiescentOrder.h"
#include "regressions/four_three/cases.h"
#include <iostream>
#include <random>
#include <stdexcept>
void require(bool ok,const char* why) { if(!ok) throw std::runtime_error(why); }
struct SearchRegression {
    static void forcedResponses() {
        Board board;
        for(int x=0;x<4;++x) require(board.place(Point(x,7),Stone::Black),"forced-response fixture");
        Evaluator evaluator; MoveGenerator generator(evaluator); Searcher search(evaluator,generator);
        evaluator.beginPosition(board);
        search.deadline_=std::chrono::steady_clock::now()+std::chrono::seconds(2);
        int win=search.negamax(board,Stone::Black,3,-SearchScore::Infinity,SearchScore::Infinity,0);
        require(win==SearchScore::Mate-1 && search.nodes_==1,"own immediate win must finish without child expansion");
        search.stats_=SearchStats{}; search.nodes_=0;
        search.negamax(board,Stone::White,1,-SearchScore::Infinity,SearchScore::Infinity,0);
        require(search.stats().forcedPrunes==1 && search.stats().quiescenceNodes==1,
                "unique mandatory block must search exactly one child");
        require(board.getHistory().size()==4,"forced response did not restore board");
    }
    static void tacticalTiers() {
        Board board;
        for(Point p:{Point(6,7),Point(8,7),Point(7,6),Point(7,8),Point(9,1),Point(10,1),Point(11,1)})
            require(board.place(p,Stone::Black),"order fixture");
        require(board.place(Point(8,1),Stone::White),"blocked four fixture");
        Evaluator evaluator; MoveGenerator generator(evaluator); Searcher search(evaluator,generator);
        const Point cross(7,7), four(12,1);
        for(Stone side:{Stone::Black,Stone::White}) {
            std::vector<Point> moves{cross,four};
            search.orderMoves(board,moves,side,0,cross);
            require(moves.front()==four,"a real four must outrank a double-three TT move");
        }
    }
    static void threatenedLeaf() {
        Board board;
        for(int x=5;x<=7;++x) require(board.place(Point(x,7),Stone::Black),"leaf threat fixture");
        Evaluator evaluator; MoveGenerator generator(evaluator); Searcher search(evaluator,generator);
        evaluator.beginPosition(board);
        search.options_.threatEvasion=true;
        search.deadline_=std::chrono::steady_clock::now()+std::chrono::seconds(2);
        const int standPat=evaluator.evaluate(board,Stone::White);
        search.quiesce(board,Stone::White,standPat-2,standPat-1,0,2);
        require(search.stats().threatEvasions>0 && search.stats().forcingMoves>0,
                "threatened leaf used stand-pat cutoff without searching a real defense");
        require(board.getHistory().size()==3,"leaf evasions failed to restore board");
    }
    static void fourThreeReplies() {
        const auto place=[](Board& board,Point move) {
            require(board.place(move,board.getCurrentPlayer()) && board.checkWin()==Stone::Empty,
                    "invalid four-three certificate position");
        };
        const auto wins=[](Board board,Stone side) {
            std::vector<Point> result;
            for(int y=0;y<15;++y) for(int x=0;x<15;++x) {
                Point move(x,y);
                if(!board.place(move,side)) continue;
                if(board.checkWin()==side) result.push_back(move);
                require(board.undo(),"four-three certificate failed to undo");
            }
            return result;
        };
        for(const auto& fixture:FourThreeFixtures::Cases) {
            Board root;
            for(Point move:fixture.prefix) place(root,move);
            const Stone defender=root.getCurrentPlayer();
            Board child=root;
            place(child,fixture.losing);
            const Stone attacker=child.getCurrentPlayer();
            require(wins(child,attacker).empty() && wins(child,defender).empty(),"unexpected five before four-three");
            Board proof=child;
            place(proof,fixture.attack);
            require(wins(proof,attacker)==std::vector<Point>{fixture.block} && wins(proof,defender).empty(),
                    "four-three first attack must force the recorded block");
            place(proof,fixture.block);
            place(proof,fixture.fork);
            require(wins(proof,attacker)==fixture.ends && wins(proof,defender).empty(),
                    "four-three certificate must produce two winning squares");

            Evaluator evaluator;
            MoveGenerator generator(evaluator);
            const auto stock=generator.generateMoves(child,attacker,12);
            const auto full=generator.generateMoves(child,attacker,225);
            Point omitted;
            evaluator.resetMoveStats();
            const auto capped=generator.generateMoves(child,attacker,12,MoveTierMode::Structural,nullptr,&omitted);
            require(capped==stock && omitted==fixture.attack && evaluator.moveStats().lineScans==0,
                    "four-three selection must reuse analysis and leave the original candidates unchanged");
            require(full.size()>=static_cast<std::size_t>(fixture.attackRank) &&
                    full[fixture.attackRank-1]==omitted &&
                    std::find(stock.begin(),stock.end(),omitted)==stock.end(),"unexpected four-three candidate ranking");
            generator.generateMoves(child,attacker,225,MoveTierMode::Structural,nullptr,&omitted);
            require(!omitted.isValid(),"four-three already in the list cannot be added twice");

            SearchOptions disabled;
            require(!disabled.retainFourThreeReply,"submission search must default to the qualified candidate policy");
            disabled.maxDepth=6;
            auto options=disabled;
            options.retainFourThreeReply=true;
            const auto searchRoot=[&](SearchOptions mode) {
                Evaluator scoring;
                MoveGenerator moves(scoring);
                Searcher engine(scoring,moves);
                return engine.search(root,defender,15000,nullptr,mode);
            };
            const auto before=searchRoot(disabled);
            const auto after=searchRoot(options);
            require(before.depth==6 && before.move==fixture.losing && before.score>=SearchScore::MateThreshold,
                    "disabled search must reproduce the four-three false win");
            require(after.status==SearchStatus::Move && after.depth==6 && after.move.isValid() &&
                    root.at(after.move)==Stone::Empty && after.move!=fixture.losing && after.score<SearchScore::MateThreshold,
                    "enabled search must avoid the proven loss and its false win");

            Searcher engine(evaluator,generator);
            const auto scoreReply=[&](SearchOptions mode) {
                evaluator.beginPosition(child);
                engine.options_=mode;
                engine.deadline_=std::chrono::steady_clock::now()+std::chrono::seconds(15);
                const int value=engine.negamax(child,attacker,5,-SearchScore::Infinity,SearchScore::Infinity,1);
                require(!engine.timeUp_,"four-three reply search did not finish");
                return value;
            };
            const int oldReply=scoreReply(disabled);
            const int newReply=scoreReply(options);
            require(oldReply<=-SearchScore::MateThreshold && newReply>=SearchScore::MateThreshold &&
                    scoreReply(disabled)==oldReply,"four-three candidate policy reused an incompatible TT score");
            std::cout<<"Four-three "<<fixture.name<<": root "<<before.move.x<<','<<before.move.y<<" -> "
                     <<after.move.x<<','<<after.move.y<<"; original reply "<<oldReply<<" -> "<<newReply<<'\n';
        }
    }
};

namespace {
using Scores = std::array<std::array<int,2>,225>;
int boards=0, tactical=0, ties=0, recursions=0;

auto forcing(const std::vector<Point>& candidates,const Scores& scores,int own) {
    std::vector<ScoredMove> result;
    for(Point p:candidates) {
        const auto pair=scores[p.y*15+p.x];
        const int a=pair[own],d=pair[1-own];
        if(d<ShapeScore::FIVE && (a>=ShapeScore::BLOCKED_FOUR || d>=ShapeScore::BLOCKED_FOUR))
            result.push_back({p,a+d*11/10});
    }
    return result;
}

void check(const Board& board,Evaluator& e) {
    if(board.checkWin()!=Stone::Empty) return;
    ++boards;
    MoveGenerator gen(e);
    for(Stone side:{Stone::Black,Stone::White}) {
        const int own=side==Stone::Black?0:1;
        Scores expected,actual;
        for(auto& row:expected) row={-1,-1};
        actual=expected;
        const auto baseline=gen.generateMoves(board,side,225,MoveTierMode::LegacyScore,&expected);
        auto candidates=collectQuiescentScores(board,e,actual);
        if(expected!=actual) throw std::runtime_error("candidate scores differ");
        const auto id=[](Point a){return a.y*15+a.x;};
        auto membership=baseline;
        std::sort(membership.begin(),membership.end(),[&](Point a,Point b){return id(a)<id(b);});
        if(membership!=candidates) throw std::runtime_error("candidate members or board order differ");
        auto slow=forcing(baseline,expected,own), fast=forcing(candidates,actual,own);
        std::stable_sort(slow.begin(),slow.end());
        sortQuiescentMoves(fast,actual,own);
        if(slow.size()!=fast.size()) throw std::runtime_error("size mismatch");
        if(slow.size()>1) ++recursions;
        for(size_t i=0;i<slow.size();++i) {
            if(slow[i].move!=fast[i].move || slow[i].score!=fast[i].score) throw std::runtime_error("forcing order differs");
            if(i && slow[i-1].score==slow[i].score) ++ties;
            ++tactical;
        }
    }
}

void checkQuiescentOrder() {
    std::mt19937 rng(20261119);
    Evaluator e;
    for(int trial=0;trial<36;++trial) {
        Board b;
        e.beginPosition(b);
        check(b,e);
        for(int turn=0;turn<54 && b.checkWin()==Stone::Empty;++turn) {
            const int span=trial%2?15:9, offset=(15-span)/2;
            Point p;
            do { const unsigned i=rng()%(span*span); p=Point(i%span+offset,i/span+offset); } while(b.at(p)!=Stone::Empty);
            require(b.place(p,b.getCurrentPlayer()),"invalid order test move");
            e.makeMove(b,p);
            if(turn%2==0) check(b,e);
        }
        int undone=0;
        while(b.undo()) {
            e.unmakeMove();
            if(++undone%3==0) check(b,e);
        }
    }
    require(boards>=1000 && ties>=100 && recursions>=100,"insufficient quiescence order test coverage");
    std::cout<<"Quiescence move order passed: "<<boards<<" positions, "<<ties<<" equal-score pairs\n";
}
}

int main() {
    checkQuiescentOrder();
    SearchRegression::threatenedLeaf();
    SearchRegression::forcedResponses();
    SearchRegression::tacticalTiers();
    SearchRegression::fourThreeReplies();
    for(int fixture=0;fixture<3;++fixture) {
        Board b;
        for(Point p:{Point(7,7),Point(8,7),Point(6,8),Point(8,8),Point(7,9),Point(9,7)}) b.place(p,b.getCurrentPlayer());
        if(fixture>0) { b.place(Point(5,6),Stone::Black); b.place(Point(9,9),Stone::White); }
        if(fixture>1) { b.place(Point(6,5),Stone::Black); b.place(Point(10,7),Stone::White); }
        Evaluator ea,ep; MoveGenerator ga(ea),gp(ep); Searcher a(ea,ga),p(ep,gp);
        auto plain=a.search(b,b.getCurrentPlayer(),10000,nullptr,{4,false,false,true});
        auto optimized=p.search(b,b.getCurrentPlayer(),10000,nullptr,{4,true,true,true});
        std::cout<<"Fixture "<<fixture<<" plain depth="<<plain.depth<<" score="<<plain.score<<" nodes="<<plain.nodes
                 <<" PVS depth="<<optimized.depth<<" score="<<optimized.score<<" nodes="<<optimized.nodes
                 <<" probes="<<p.stats().nullWindowProbes<<" researches="<<p.stats().researches
                 <<" killers="<<p.stats().killerHits<<" history="<<p.stats().historyUpdates<<'\n';
        require(plain.depth==optimized.depth && plain.depth>0,"fixed-depth comparison incomplete");
        require(plain.score==optimized.score,"PVS changed minimax score");
        require(optimized.move.isValid() && b.at(optimized.move)==Stone::Empty,"PVS legal move");
        require(p.stats().nullWindowProbes>0,"PVS not exercised");
        if(optimized.score<SearchScore::MateThreshold)
            require(p.stats().historyUpdates>0,"history not exercised on non-winning fixture");
        require(p.stats().tacticalPriorityFirst>0,"search did not prioritize a tactical tier");
    }
    for(int transform=0;transform<4;++transform) {
        Board threat;
        const Stone attacker=transform%2 ? Stone::White : Stone::Black;
        const Stone defender=attacker==Stone::Black?Stone::White:Stone::Black;
        auto point=[&](int x,int y) { return transform<2 ? Point(x,y) : Point(y,x); };
        for(int x=5;x<=7;++x) require(threat.place(point(x,7),attacker),"open three fixture");
        Evaluator defenseEval; MoveGenerator defenseGen(defenseEval); Searcher defenseSearch(defenseEval,defenseGen);
        SearchOptions defenseOptions; defenseOptions.maxDepth=1; defenseOptions.quiescenceDepth=4;
        auto defended=defenseSearch.search(threat,defender,2000,nullptr,defenseOptions);
        require(defended.move==point(4,7) || defended.move==point(8,7),"shallow search must prevent opponent open four");
        std::cout<<"Defense fixture "<<transform<<" move="<<defended.move.x<<','<<defended.move.y
                 <<" score="<<defended.score<<" evasions="<<defenseSearch.stats().threatEvasions<<'\n';
    }
    Board fallbackBoard; fallbackBoard.place(Point(7,7),Stone::Black);
    Evaluator ef; MoveGenerator gf(ef); Searcher sf(ef,gf);
    SearchOptions fallbackOptions; fallbackOptions.maxDepth=1; fallbackOptions.rootCandidates=0;
    auto fallback=sf.search(fallbackBoard,Stone::White,500,nullptr,fallbackOptions);
    require(fallback.status==SearchStatus::Move && fallback.reason==SearchReason::Fallback && fallback.move.isValid(),"empty candidate fallback");
    require(fallbackBoard.place(fallback.move,Stone::White),"fallback must be legal");
    std::atomic<bool> cancelled{true};
    auto stopped=sf.search(fallbackBoard,Stone::Black,500,&cancelled);
    require(stopped.status==SearchStatus::Cancelled && !stopped.move.isValid(),"explicit cancelled status");
    Board doomed;
    for(int x=4;x<=7;++x) require(doomed.place(Point(x,7),Stone::Black),"double-end threat setup");
    Evaluator ed; MoveGenerator gd(ed); Searcher resistance(ed,gd);
    SearchOptions lossOptions; lossOptions.maxDepth=3; lossOptions.quiescence=true; lossOptions.quiescenceDepth=4;
    auto loss=resistance.search(doomed,Stone::White,2500,nullptr,lossOptions);
    require(loss.score<=-SearchScore::MateThreshold,"quiescence must see the unavoidable double-end loss");
    require(loss.depth==lossOptions.maxDepth,"search must keep deepening after a proven loss");
    require(resistance.stats().quiescenceNodes>0,"double-end horizon must enter quiescence");

    Board forcing;
    for(int x=4;x<=6;++x) require(forcing.place(Point(x,7),Stone::White),"forcing setup white");
    require(forcing.place(Point(3,7),Stone::Black),"forcing setup block");
    Evaluator eq; MoveGenerator gq(eq); Searcher qsearch(eq,gq);
    SearchOptions qOptions; qOptions.maxDepth=1; qOptions.quiescence=true; qOptions.quiescenceDepth=4;
    auto qresult=qsearch.search(forcing,Stone::White,1500,nullptr,qOptions);
    std::cout<<"Quiescence nodes="<<qsearch.stats().quiescenceNodes
             <<" forced blocks="<<qsearch.stats().forcedBlocks
             <<" forcing moves="<<qsearch.stats().forcingMoves<<'\n';
    require(qresult.status==SearchStatus::Move && qresult.move.isValid(),"quiescence legal result");
    require(qsearch.stats().quiescenceNodes>0,"leaf quiescence was not entered");
    require(qsearch.stats().forcedBlocks>0,"quiescence did not follow the single winning-point block");
    require(qresult.nodes>=qsearch.stats().quiescenceNodes,"total nodes omit quiescence work");
    SearchOptions hostileQ=qOptions; hostileQ.quiescenceDepth=1000000; hostileQ.maxDepth=10;
    auto boundedStart=std::chrono::steady_clock::now();
    auto bounded=qsearch.search(forcing,Stone::White,100,nullptr,hostileQ);
    auto boundedMs=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-boundedStart).count();
    require(bounded.status==SearchStatus::Move && bounded.move.isValid(),"clamped quiescence must return legal fallback");
    require(boundedMs<=250,"clamped quiescence exceeded bounded time");
    require(qsearch.stats().quiescenceNodes<100000,"quiescence depth was not bounded");
    std::cout<<"Hostile q-depth request returned in "<<boundedMs<<"ms with "<<qsearch.stats().quiescenceNodes<<" qnodes\n";
    SearchOptions noQ=qOptions; noQ.quiescence=false;
    Evaluator es; MoveGenerator gs(es); Searcher staticSearch(es,gs);
    auto staticResult=staticSearch.search(forcing,Stone::White,1500,nullptr,noQ);
    require(staticSearch.stats().quiescenceNodes==0,"disabled quiescence still expanded nodes");
    require(staticResult.status==SearchStatus::Move,"static baseline must remain usable");
    std::cout<<"Horizon fixture static="<<staticResult.score<<" quiescent="<<qresult.score<<'\n';
    Board switching;
    for(Point p:{Point(7,7),Point(8,7),Point(6,8),Point(8,8),Point(7,9),Point(9,7)})
        require(switching.place(p,switching.getCurrentPlayer()),"mode-switch fixture");
    Evaluator reusedEval; MoveGenerator reusedGen(reusedEval); Searcher reused(reusedEval,reusedGen);
    for(int mode:{4,-1,1,4}) {
        SearchOptions configuration; configuration.maxDepth=2;
        configuration.quiescence=mode>=0;
        configuration.quiescenceDepth=mode>=0 ? mode : 0;
        auto warmed=reused.search(switching,switching.getCurrentPlayer(),5000,nullptr,configuration);
        Evaluator freshEval; MoveGenerator freshGen(freshEval); Searcher fresh(freshEval,freshGen);
        auto reference=fresh.search(switching,switching.getCurrentPlayer(),5000,nullptr,configuration);
        require(warmed.depth==2 && reference.depth==2,"mode-switch comparison incomplete");
        require(warmed.score==reference.score && warmed.move==reference.move,"TT reused incompatible leaf configuration");
    }
    std::cout<<"Leaf configuration cache isolation passed\n";
    std::cout<<"Bounded quiescence and static-leaf control passed\n";
    std::cout<<"PVS/killer/history and fallback regressions passed\n";
}
