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
    std::cout<<"Move generation cache regressions passed\n";
}
