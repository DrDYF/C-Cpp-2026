#include "ai/MoveGen.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <random>
#include <set>
#include <stdexcept>

void require(bool ok,const char* why) { if(!ok) throw std::runtime_error(why); }
void verify(const Board& board) {
    std::set<int> actual;
    std::vector<Point> candidates;
    board.appendCandidates(candidates);
    for(Point p:candidates) {
        const int id=p.y*15+p.x;
        require(board.at(p)==Stone::Empty,"candidate must be empty");
        require(actual.insert(id).second,"candidate duplicate");
    }
    const auto& history=board.getHistory();
    for(int y=0;y<15;++y) for(int x=0;x<15;++x) {
        Point p(x,y); int count=0;
        for(Point stone:history)
            if(p!=stone && std::max(std::abs(stone.x-x),std::abs(stone.y-y))<=BoardConst::CANDIDATE_RADIUS) ++count;
        require(board.nearbyStoneCount(p)==count,"incremental neighbor count differs from full recomputation");
        const bool expected=board.at(p)==Stone::Empty &&
            (history.empty() ? p==Point(7,7) : count>0);
        if(board.isCandidate(p)!=expected) {
            std::cerr<<"candidate mismatch at "<<x<<','<<y<<" count="<<count
                     <<" occupied="<<(board.at(p)!=Stone::Empty)<<" history="<<history.size()<<'\n';
            throw std::runtime_error("incremental candidate set differs from full recomputation");
        }
        require(actual.count(y*15+x)==expected,"enumerated candidate bitset differs from invariant");
    }
}
int main() {
    Board board; verify(board);
    std::mt19937 rng(0xCA7D1);
    for(int game=0;game<32;++game) {
        board.reset(); verify(board);
        for(int ply=0;ply<90 && board.checkWin()==Stone::Empty;++ply) {
            Point p(static_cast<int>(rng()%15),static_cast<int>(rng()%15));
            if(board.place(p,board.getCurrentPlayer())) {
                verify(board);
                Board copy=board; verify(copy);
            }
        }
        while(board.undo()) verify(board);
        verify(board);
    }

    Board sample;
    for(int i=0;i<60 && sample.checkWin()==Stone::Empty;++i) {
        Point p(static_cast<int>(rng()%15),static_cast<int>(rng()%15));
        sample.place(p,sample.getCurrentPlayer());
    }
    auto rebuild=[&]() {
        std::array<bool,225> nearby{};
        if(sample.getHistory().empty()) nearby[7*15+7]=true;
        else for(Point stone:sample.getHistory())
            for(int dy=-2;dy<=2;++dy) for(int dx=-2;dx<=2;++dx) {
                Point p(stone.x+dx,stone.y+dy);
                if(p.isValid()) nearby[p.y*15+p.x]=true;
            }
        std::vector<Point> out;
        for(int y=0;y<15;++y) for(int x=0;x<15;++x)
            if(nearby[y*15+x] && sample.at(Point(x,y))==Stone::Empty) out.emplace_back(x,y);
        return out;
    };
    std::vector<Point> expected=rebuild(), incremental;
    sample.appendCandidates(incremental);
    require(expected==incremental,"incremental enumeration must match previous full rebuild");
    constexpr int iterations=3000;
    volatile size_t checksum=0;
    auto t0=std::chrono::steady_clock::now();
    for(int i=0;i<iterations;++i) checksum+=rebuild().size();
    auto rebuildUs=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-t0).count();
    t0=std::chrono::steady_clock::now();
    for(int i=0;i<iterations;++i) { sample.appendCandidates(incremental); checksum+=incremental.size(); }
    auto bitsetUs=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-t0).count();
    std::cout<<"Candidate enumeration at "<<sample.getHistory().size()<<" stones ("<<iterations<<" rounds): rebuild="<<rebuildUs
             <<"us bitset="<<bitsetUs<<"us checksum="<<checksum<<'\n';

    Board tactical;
    for(int x=3;x<=6;++x) {
        require(tactical.place(Point(x,7),Stone::Black),"black four setup");
        require(tactical.place(Point(x,1),Stone::White),"white four setup");
    }
    Evaluator evaluator; MoveGenerator generator(evaluator);
    const auto moves=generator.generateMoves(tactical,Stone::Black,1);
    bool ownWin=false, blockOtherWin=false;
    for(Point p:moves) {
        ownWin |= p==Point(7,7);
        blockOtherWin |= p==Point(7,1);
    }
    require(ownWin && blockOtherWin,"maxCount must not prune immediate wins or their blocks");
    require(moves.front()==Point(7,7),"our immediate win must outrank defensive blocks");

    Board block;
    for(int x=3;x<=6;++x) require(block.place(Point(x,7),Stone::Black),"opponent four setup");
    require(block.place(Point(2,7),Stone::White),"close one end");
    const auto responses=generator.generateMoves(block,Stone::White,1);
    require(!responses.empty() && responses.front()==Point(7,7),"mandatory block must be first even at cap one");
    Board doubleThree;
    for(Point p:{Point(6,7),Point(8,7),Point(7,6),Point(7,8)})
        require(doubleThree.place(p,Stone::Black),"double three setup");
    const auto structure=evaluator.moveThreats(doubleThree,Point(7,7))[0];
    require(evaluator.evaluateMove(doubleThree,Point(7,7),Stone::Black)>=ShapeScore::BLOCKED_FOUR,
            "fixture must exceed old scalar forcing threshold");
    require(structure.threes==2 && !structure.forcing(),"double three must not become a forcing four");
    Board realFour;
    for(int x=4;x<=6;++x) require(realFour.place(Point(x,7),Stone::Black),"real four setup");
    const auto four=evaluator.moveThreats(realFour,Point(7,7))[0];
    require(four.fours==1 && four.openFours==1 && four.decisiveThreat(),"actual open four lost structural classification");
    std::cout<<"Incremental candidate invariants and tactical tier regressions passed\n";
}
