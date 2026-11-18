#include "ai/Evaluator.h"
#include <random>
#include <stdexcept>
#include <iostream>
#include <chrono>
void check(Evaluator& e,const Board& b) {
    if(e.evaluate(b,Stone::Black)!=e.evaluateFull(b,Stone::Black) ||
       e.evaluate(b,Stone::White)!=e.evaluateFull(b,Stone::White)) throw std::runtime_error("incremental evaluation mismatch");
}
int main() {
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
