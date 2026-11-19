#include "ai/Evaluator.h"
#include <random>
#include <stdexcept>
#include <iostream>
#include <chrono>
void check(Evaluator& e,const Board& b) {
    if(e.evaluate(b,Stone::Black)!=e.evaluateFull(b,Stone::Black) ||
       e.evaluate(b,Stone::White)!=e.evaluateFull(b,Stone::White)) throw std::runtime_error("incremental evaluation mismatch");
}
void checkMovePairCache() {
    Board b;
    Evaluator e;
    const Point p(7,7), extension(8,7);
    for(Point stone:{Point(4,7),Point(5,7),Point(6,7)})
        if(!b.place(stone,Stone::Black)) throw std::runtime_error("invalid cache fixture");
    e.beginPosition(b);
    auto checkPair=[&](const Board& board,long long scans,long long hits,bool five) {
        e.resetMoveStats();
        const std::array<int,2> expected{e.evaluateMoveReference(board,p,Stone::Black),
                                         e.evaluateMoveReference(board,p,Stone::White)};
        if(e.evaluateMovePair(board,p)!=expected || e.evaluateMovePair(board,p)!=expected)
            throw std::runtime_error("move pair cache mismatch");
        const auto stats=e.moveStats();
        if(stats.lineScans!=scans || stats.cacheHits!=hits)
            throw std::runtime_error("move pair cache counters mismatch");
        if(e.moveThreats(board,p)[0].five!=five)
            throw std::runtime_error("move pair cache threat mismatch");
    };
    checkPair(b,4,4,false);
    if(!b.place(extension,Stone::Black)) throw std::runtime_error("invalid cache extension");
    e.makeMove(b,extension);
    checkPair(b,1,7,true);
    if(!b.undo()) throw std::runtime_error("cache fixture undo failed");
    e.unmakeMove();
    checkPair(b,1,7,false);
    Board other;
    checkPair(other,4,4,false);
    checkPair(b,4,4,false);
}
int main() {
    checkMovePairCache();
    Evaluator e; std::mt19937 rng(19);
    for(int game=0;game<40;++game) {
        Board b; e.beginPosition(b);
        for(int step=0;step<70 && b.checkWin()==Stone::Empty;++step) {
            Point p(rng()%15,rng()%15);
            if(b.place(p,b.getCurrentPlayer())) { e.makeMove(b,p); check(e,b); }
        }
        while(b.undo()) { e.unmakeMove(); check(e,b); }
    }
    Board b;
    for(Point p:{Point(7,7),Point(8,7),Point(6,8),Point(8,8),Point(7,9),Point(9,7)}) b.place(p,b.getCurrentPlayer());
    e.beginPosition(b);
    volatile int sink=0;
    auto start=std::chrono::steady_clock::now();
    for(int i=0;i<5000;++i) sink=e.evaluateFull(b,Stone::Black);
    auto full=std::chrono::steady_clock::now()-start;
    start=std::chrono::steady_clock::now();
    for(int i=0;i<5000;++i) sink=e.evaluate(b,Stone::Black);
    auto cached=std::chrono::steady_clock::now()-start;
    std::cout<<"Full evaluations us="<<std::chrono::duration_cast<std::chrono::microseconds>(full).count()
             <<" cached us="<<std::chrono::duration_cast<std::chrono::microseconds>(cached).count()<<'\n';
    std::cout<<"Incremental evaluation regressions passed (40 random games, make and unmake)\n";
}
