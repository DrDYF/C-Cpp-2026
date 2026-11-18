#include "ai/MoveGen.h"
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
