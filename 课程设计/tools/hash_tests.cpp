#include "core/Board.h"
#include "core/Zobrist.h"
#include <iostream>
#include <random>
#include <stdexcept>
int main() {
    std::mt19937 rng(42);
    for(int game=0;game<40;++game) {
        Board b;
        for(int i=0;i<100 && b.checkWin()==Stone::Empty;++i) {
            Point p(static_cast<int>(rng()%15),static_cast<int>(rng()%15));
            b.place(p,b.getCurrentPlayer());
            if(b.getHash()!=Zobrist::hash(b)) throw std::runtime_error("make hash");
            Board copy=b;
            if(copy.getHash()!=b.getHash()) throw std::runtime_error("copy hash");
        }
        while(b.undo()) if(b.getHash()!=Zobrist::hash(b)) throw std::runtime_error("undo hash");
        if(b.getHash()!=0) throw std::runtime_error("empty hash");
        b.place(Point(7,7),Stone::Black); b.reset();
        if(b.getHash()!=0) throw std::runtime_error("reset hash");
    }
    Board won;
    for(int x=0;x<5;++x) if(!won.place(Point(x,0),Stone::Black)) throw std::runtime_error("win setup");
    if(won.checkWin()!=Stone::Black || won.place(Point(9,9),Stone::White)) throw std::runtime_error("terminal board");
    while(won.undo()) if(won.checkWin()!=Stone::Empty) throw std::runtime_error("undo winner");
    if(!won.place(Point(7,7),Stone::Black)) throw std::runtime_error("play after undo");
    std::cout<<"Incremental hash and undo winner regressions passed\n";
}
