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
void retainsCounterFour() {
    // axial-10-1：第 55 手黑走 (11,4)，白反冲 (5,13) 可形成两个胜点。
    const std::array<Point,54> prefix={{
        {7,7},{8,7},{9,6},{10,6},{8,5},{10,7},
        {7,6},{9,7},{9,4},{6,7},{7,5},{7,4},
        {9,5},{6,5},{11,5},{10,5},{10,3},{11,2},
        {9,2},{9,3},{10,8},{6,8},{6,6},{5,6},
        {8,3},{7,8},{8,9},{8,8},{5,8},{7,9},
        {6,10},{5,7},{4,6},{4,7},{3,8},{11,7},
        {12,7},{8,10},{9,11},{7,11},{3,7},{5,5},
        {4,5},{8,2},{10,4},{5,4},{5,3},{7,10},
        {7,12},{6,4},{4,4},{7,3},{9,1},{6,12}
    }};
    Board board;
    for(Point p:prefix)
        require(board.place(p,board.getCurrentPlayer()),"invalid axial-10-1 replay");
    require(board.place(Point(11,4),Stone::Black) && board.checkWin()==Stone::Empty &&
            board.getCurrentPlayer()==Stone::White,"invalid axial-10-1 move 55");

    const Point reply(5,13);
    Board after=board;
    require(after.place(reply,Stone::White) && after.checkWin()==Stone::Empty,"invalid white counter-four");
    for(Point endpoint:{Point(9,9),Point(4,14)}) {
        Board finish=after;
        require(finish.place(endpoint,Stone::White) && finish.checkWin()==Stone::White,
                "counter-four endpoint must complete five");
    }
    for(int y=0;y<BoardConst::SIZE;++y) for(int x=0;x<BoardConst::SIZE;++x) {
        const Point p(x,y);
        if(after.at(p)!=Stone::Empty) continue;
        Board black=after;
        require(black.place(p,Stone::Black) && black.checkWin()!=Stone::Black,
                "black must not have an immediate counter-win");
    }

    Evaluator evaluator; MoveGenerator generator(evaluator);
    const auto candidates=generator.generateMoves(board,Stone::White,12);
    require(std::find(candidates.begin(),candidates.end(),reply)!=candidates.end(),
            "axial-10-1: internal candidates omitted white counter-four (5,13)");
}

bool retainsForkReply(const std::vector<Point>& history,Point reply,
                      const std::array<Point,2>& endpoints,const char* name) {
    Board board;
    for(Point p:history)
        require(board.place(p,board.getCurrentPlayer()),"invalid fork regression replay");
    require(board.getCurrentPlayer()==Stone::White && board.checkWin()==Stone::Empty,
            "expected a nonterminal position with white to move");

    Board after=board;
    require(after.place(reply,Stone::White) && after.checkWin()==Stone::Empty,"invalid fork reply");
    for(Point endpoint:endpoints) {
        Board finish=after;
        require(finish.place(endpoint,Stone::White) && finish.checkWin()==Stone::White,
                "fork endpoint must complete five");
    }
    for(int y=0;y<BoardConst::SIZE;++y) for(int x=0;x<BoardConst::SIZE;++x) {
        const Point p(x,y);
        if(after.at(p)!=Stone::Empty) continue;
        Board black=after;
        require(black.place(p,Stone::Black) && black.checkWin()!=Stone::Black,
                "black must not have an immediate counter-win");
    }

    Evaluator evaluator; MoveGenerator generator(evaluator);
    const auto candidates=generator.generateMoves(board,Stone::White,12);
    if(std::find(candidates.begin(),candidates.end(),reply)!=candidates.end()) return true;
    std::cerr<<name<<": cap 12 omitted ("<<reply.x<<','<<reply.y<<")\n";
    return false;
}

void verifyForkReplies() {
    const bool first=retainsForkReply({
        {7,7},{8,8},{6,6},{4,7},{9,9},{4,8},
        {9,7},{4,6},{4,5},{6,8},{8,7},{5,8},
        {7,8},{3,8},{2,8},{4,9},{4,10},{6,7},
        {7,6},{3,10},{2,11},{7,9},{9,6}
    },Point(5,7),{Point(3,5),Point(8,10)},"diagonal-06-3");
    const bool second=retainsForkReply({
        {7,7},{8,8},{9,6},{7,5},{10,9},{8,6},
        {8,7},{9,7},{10,8},{10,6},{11,5},{9,8},
        {9,9},{7,8},{8,10},{7,9},{6,10},{7,11},
        {11,7},{12,6},{7,10},{9,10},{5,10},{4,10},
        {11,6},{5,8},{6,8},{11,8},{10,5},{11,4},
        {12,5},{9,5},{9,4},{12,7},{6,7},{6,9},
        {6,4},{4,9},{5,9},{4,8},{4,7},{5,7},
        {6,6},{6,5},{8,5},{4,11},{4,12},{5,11},
        {7,4}
    },Point(6,11),{Point(3,11),Point(8,11)},"diagonal-09-2");
    require(first && second,"cap 12 must retain both fork replies");
}

int main() {
    retainsCounterFour();
    verifyForkReplies();
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
