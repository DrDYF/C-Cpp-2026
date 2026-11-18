#include "ai/AIEngine.h"
#include "ai/TranspositionTable.h"
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <algorithm>
void require(bool ok,const char* why) { if(!ok) throw std::runtime_error(why); }
int main() {
    TranspositionTable tt(1);
    int score; Point move; TTEntry::Flag flag;
    tt.store(123,8,SearchScore::Mate-9,Point(7,7),TTEntry::EXACT,4);
    require(tt.probe(123,8,score,move,flag,2) && score==SearchScore::Mate-7,"positive mate normalization");
    tt.store(124,8,-SearchScore::Mate+9,Point(7,7),TTEntry::EXACT,4);
    require(tt.probe(124,8,score,move,flag,2) && score==-SearchScore::Mate+7,"negative mate normalization");
    require(!tt.probe(125,1,score,move,flag),"TT collision must miss");
    Board win;
    for(int x=3;x<7;++x) { require(win.place(Point(x,7),Stone::Black),"setup B"); require(win.place(Point(x*2,0),Stone::White),"setup W"); }
    AIEngine engine;
    auto result=engine.think(win,500);
    Board played=win;
    require(played.place(result.move,Stone::Black) && played.checkWin()==Stone::Black,"take immediate win");
    Board defend=win; defend.undo();
    result=engine.think(defend,500);
    require(result.move==Point(2,7) || result.move==Point(7,7),"legal best resistance to open four");
    require(result.score<=-SearchScore::MateThreshold && result.depth>=1 && result.nodes>436,"multiple winning points must search and report loss");
    Board counterWin;
    for(int x=3;x<7;++x) { counterWin.place(Point(x,7),Stone::Black); counterWin.place(Point(x,10),Stone::White); }
    counterWin.place(Point(0,0),Stone::Black);
    result=engine.think(counterWin,500);
    require(counterWin.place(result.move,Stone::White) && counterWin.checkWin()==Stone::White,"own immediate win beats blocking double threat");
    Board uniqueBlock;
    for(int x=0;x<4;++x) uniqueBlock.place(Point(x,7),Stone::Black);
    for(int x:{0,2,4}) uniqueBlock.place(Point(x,0),Stone::White);
    result=engine.think(uniqueBlock,500);
    require(result.move==Point(4,7) && result.reason==SearchReason::ForcedDefense,"mandatory-block reason");
    uniqueBlock.place(result.move,Stone::White);
    Evaluator defendedEval;
    require(result.score==defendedEval.evaluateFull(uniqueBlock,Stone::White),"mandatory block must score defended board");
    Evaluator eval; MoveGenerator gen(eval); Searcher search(eval,gen);
    for(int i=0;i<3;++i) {
        result=search.search(win,Stone::Black,500);
        require(result.score>=SearchScore::MateThreshold,"negamax win sign across TT reuse");
        Board p=win; require(p.place(result.move,Stone::Black) && p.checkWin()==Stone::Black,"search mate move");
    }
    Board three;
    for(int x=6;x<9;++x) three.place(Point(x,7),Stone::Black);
    auto moves=gen.generateMoves(three,Stone::White,3);
    require(std::find(moves.begin(),moves.end(),Point(5,7))!=moves.end() ||
            std::find(moves.begin(),moves.end(),Point(9,7))!=moves.end(),"top-three defense against open three");
    Board middle;
    for(Point p:{Point(7,7),Point(8,7),Point(6,8),Point(8,8),Point(7,9),Point(9,7),Point(6,6),Point(9,9)})
        require(middle.place(p,middle.getCurrentPlayer()),"middle setup");
    for(int i=0;i<10;++i) {
        auto start=std::chrono::steady_clock::now();
        result=engine.think(middle,500);
        auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count();
        std::cout<<"500ms call "<<i<<": "<<ms<<"ms depth="<<result.depth<<" nodes="<<result.nodes<<'\n';
        require(ms<=700,"deadline exceeded 700ms");
        require(result.move.isValid() && middle.at(result.move)==Stone::Empty,"legal timed move");
    }
    std::atomic<bool> cancel{true};
    result=engine.think(middle,1500,&cancel);
    require(!result.move.isValid(),"already-cancelled engine call");
    std::cout<<"Engine regressions passed\n";
}
