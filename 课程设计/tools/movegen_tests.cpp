#include "ai/MoveGen.h"
#include "regressions/four_three/cases.h"
#include <algorithm>
#include <iostream>
#include <random>
#include <stdexcept>
void require(bool v,const char* why) { if(!v) throw std::runtime_error(why); }
void check(Evaluator& e,const Board& b) {
    for(int y=0;y<15;++y) for(int x=0;x<15;++x) if(b.at(Point(x,y))==Stone::Empty) {
        Point p(x,y); auto pair=e.evaluateMovePair(b,p);
        require(pair[0]==e.evaluateMoveReference(b,p,Stone::Black),"black move cache stale");
        require(pair[1]==e.evaluateMoveReference(b,p,Stone::White),"white move cache stale");
    }
}
void forcingFourCandidates(Evaluator& evaluator,MoveGenerator& gen) {
    bool addedPlainFour=false;
    for(const auto& fixture:FourThreeFixtures::Cases) {
        Board board;
        for(Point p:fixture.prefix)
            require(board.place(p,board.getCurrentPlayer()),"invalid forcing-four fixture");
        require(board.place(fixture.losing,board.getCurrentPlayer()),"invalid forcing-four fixture reply");
        for(Stone side:{Stone::Black,Stone::White}) {
            const int own=side==Stone::Black?0:1;
            const auto baseline=gen.generateMoves(board,side,12);
            const auto full=gen.generateMoves(board,side,225);
            std::vector<Point> expected;
            for(Point p:full) {
                const auto attack=evaluator.moveThreats(board,p)[own];
                const bool inBaseline=std::find(baseline.begin(),baseline.end(),p)!=baseline.end();
                if(inBaseline || attack.fours>0) expected.push_back(p);
                if(inBaseline || attack.fours==0) continue;
                Board played=board;
                require(played.place(p,side) && played.checkWin()==Stone::Empty,"extra four must be a legal non-winning move");
                bool createsWin=false;
                for(int y=0;y<15 && !createsWin;++y) for(int x=0;x<15 && !createsWin;++x) {
                    if(!played.place(Point(x,y),side)) continue;
                    createsWin=played.checkWin()==side;
                    require(played.undo(),"could not undo four completion");
                }
                require(createsWin,"extra four must create a real winning point");
                if(attack.threes==0) addedPlainFour=true;
            }
            evaluator.resetMoveStats();
            const auto widened=gen.generateMoves(board,side,12,MoveTierMode::Structural,nullptr,nullptr,
                                                 MoveTailPolicy::KeepForcingFours);
            require(widened==expected && evaluator.moveStats().lineScans==0,
                    "forcing-four tail must preserve ranking and reuse cached analysis");
            if(side==board.getCurrentPlayer())
                require(std::find(widened.begin(),widened.end(),fixture.attack)!=widened.end(),
                        "forcing-four tail omitted the certified attack");
        }
    }
    require(addedPlainFour,"forcing-four rule must also exercise a four without a live three");
}
void stagedOpenFour(MoveGenerator& gen) {
    const auto winCount=[](Board board,Stone side) {
        int count=0;
        for(int y=0;y<15;++y) for(int x=0;x<15;++x) {
            if(!board.place({x,y},side)) continue;
            count+=board.checkWin()==side;
            require(board.undo(),"could not undo winning-point trial");
        }
        return count;
    };
    for(Stone side:{Stone::Black,Stone::White}) {
        const Stone opponent=side==Stone::Black?Stone::White:Stone::Black;
        const bool transposed=side==Stone::White;
        const auto point=[transposed](int x,int y) { return transposed?Point(y,x):Point(x,y); };
        const auto id=[](Point p) { return p.y*15+p.x; };
        const Point counter=point(3,1), jump=point(4,1);
        Board board;
        for(int x=0;x<3;++x) require(board.place(point(x,1),side),"counter-four fixture");
        MoveMask priority;
        auto candidates=[&] { return gen.generateMoves(board,side,1,MoveTierMode::Structural,nullptr,nullptr,
                                                        MoveTailPolicy::DefendOpenFour,&priority); };
        require(candidates()==gen.generateMoves(board,side,1) && priority.none(),
                "own four alone must not trigger the defense stage");
        for(int x=5;x<=7;++x) require(board.place(point(x,7),opponent),"open-four threat fixture");
        for(bool secondLine:{false,true}) {
            if(secondLine)
                for(int x=5;x<=7;++x) require(board.place(point(x,12),opponent),"second open-four threat fixture");
            const auto baseline=gen.generateMoves(board,side,1);
            require(std::find(baseline.begin(),baseline.end(),counter)==baseline.end(),
                    "baseline must omit the remote counter-four");
            const auto staged=candidates();
            MoveMask expected;
            expected.set(id(counter));
            expected.set(id(jump));
            if(!secondLine) { expected.set(id(point(4,7))); expected.set(id(point(8,7))); }
            require(priority==expected,"incorrect open-four defense set");
            MoveMask appeared;
            bool reachedOrdinary=false;
            for(Point move:staged) {
                require(!appeared.test(id(move)),"duplicate staged move");
                appeared.set(id(move));
                if(!priority.test(id(move))) {
                    reachedOrdinary=true;
                    require(std::find(baseline.begin(),baseline.end(),move)!=baseline.end(),
                            "stage added an ordinary move");
                    continue;
                }
                require(!reachedOrdinary,"a defense came after an ordinary move");
                Board defended=board;
                require(defended.place(move,side),"illegal staged move");
                if(move==counter || move==jump) {
                    require(winCount(defended,side)==1,"counter-four must really threaten five");
                } else {
                    for(int y=0;y<15;++y) for(int x=0;x<15;++x) {
                        if(!defended.place({x,y},opponent)) continue;
                        require(winCount(defended,opponent)<=1,"block left an opponent open-four reply");
                        require(defended.undo(),"could not undo threat trial");
                    }
                }
            }
            require((appeared&expected)==expected,"stage omitted a required defense or counter-four");
        }
        require(board.place(point(8,7),opponent) && board.place(point(9,7),side),"immediate win fixture");
        const auto urgent=candidates();
        require(priority.none() && !urgent.empty() && urgent.front()==point(4,7),
                "open-four defense stage must not displace a mandatory five block");
    }
}
int main() {
    Evaluator e; MoveGenerator gen(e); std::mt19937 rng(22);
    for(int game=0;game<12;++game) {
        Board b; e.beginPosition(b); check(e,b);
        for(int k=0;k<35 && b.checkWin()==Stone::Empty;++k) {
            Point p(rng()%15,rng()%15);
            if(b.place(p,b.getCurrentPlayer())) { e.makeMove(b,p); check(e,b); }
        }
        while(b.undo()) { e.unmakeMove(); check(e,b); }
    }
    Board b; b.place(Point(7,7),Stone::Black); b.place(Point(8,7),Stone::White);
    e.beginPosition(b); e.resetMoveStats();
    auto first=gen.generateMoves(b,Stone::Black);
    auto cold=e.moveStats(); e.resetMoveStats();
    auto second=gen.generateMoves(b,Stone::Black); auto warm=e.moveStats();
    require(first==second && warm.lineScans==0 && warm.cacheHits>0,"repeated move generation rescanned lines");
    std::cout<<"Cold direction reads="<<cold.lineScans<<" repeated reads="<<warm.lineScans<<" reused="<<warm.cacheHits<<'\n';
    Board threats;
    for(Point p:{Point(6,7),Point(8,7),Point(7,6),Point(7,8),Point(9,1),Point(10,1),Point(11,1)})
        require(threats.place(p,Stone::Black),"threat tier fixture");
    require(threats.place(Point(8,1),Stone::White),"blocked four fixture");
    const Point cross(7,7), four(12,1);
    const auto doubleThree=e.moveThreats(threats,cross)[0];
    const auto blockedFour=e.moveThreats(threats,four)[0];
    require(doubleThree.threes==2 && !doubleThree.forcing(),"expected a double-three without a four");
    require(blockedFour.fours==1 && blockedFour.openFours==0 && !blockedFour.five,"expected a blocked four");
    require(e.evaluateMove(threats,cross,Stone::Black)>e.evaluateMove(threats,four,Stone::Black),
            "fixture must expose the score/shape mismatch");
    for(Stone side:{Stone::Black,Stone::White}) {
        const auto capped=gen.generateMoves(threats,side,1);
        require(!capped.empty() && capped.front()==four,"a real four must outrank the higher-scoring double-three");
        for(Point p:capped) require(p!=cross,"double-three must not bypass the candidate cap");
    }

    const auto legacy=gen.generateMoves(threats,Stone::White,225,MoveTierMode::LegacyScore);
    require(!legacy.empty() && legacy.front()==cross,"leaf ordering must still prioritize the legacy score tier");
    forcingFourCandidates(e,gen);
    stagedOpenFour(gen);

    Board sameLine;
    for(int x:{3,5,7,9}) require(sameLine.place(Point(x,7),Stone::Black),"same-line double-four fixture");
    const Point split(6,7);
    const auto splitThreat=e.moveThreats(sameLine,split)[0];
    require(!splitThreat.five && splitThreat.fours>0,"two same-line winning points must qualify for the four tier");
    require(sameLine.place(split,Stone::Black) && sameLine.checkWin()==Stone::Empty,"split must not already win");
    for(int x:{4,8}) {
        require(sameLine.place(Point(x,7),Stone::Black) && sameLine.checkWin()==Stone::Black,
                "both same-line completions must win");
        require(sameLine.undo(),"restore same-line fixture");
    }
    std::cout<<"Move generation cache regressions passed\n";
}
