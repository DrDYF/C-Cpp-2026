#include "ai/Pattern.h"
#include "ai/Evaluator.h"
#include <iostream>
#include <stdexcept>
#include <string>
void require(bool ok,const char* why) { if(!ok) throw std::runtime_error(why); }
int main() {
    PatternRecognizer patterns;
    auto check=[&](const char* text, ShapeType expected) {
        int8_t line[9];
        for(int i=0;i<9;++i) line[i]=text[i]=='X'?1:text[i]=='.'?0:2;
        auto got=patterns.recognizeLine(line,1);
        std::cout<<text<<" -> "<<static_cast<int>(got)<<'\n';
        require(got==expected,text);
    };
    check("..XXXX...",ShapeType::FOUR);
    check("..X.XXX..",ShapeType::BLOCKED_FOUR);
    check(".XX.XX...",ShapeType::BLOCKED_FOUR);
    check("..XXX.X..",ShapeType::BLOCKED_FOUR);
    check("...XXX...",ShapeType::THREE);
    check(".XX.X....",ShapeType::THREE);
    check("X.X.X....",ShapeType::SLEEP_THREE);
    check("##XXXX###",ShapeType::NONE);
    int8_t three[9]={0,0,0,1,1,1,0,0,0};
    int8_t four[9]={0,0,1,1,1,1,0,0,0};
    require(patterns.scoreLine(three,9,1)==ShapeScore::THREE,"deduplicate open three");
    require(patterns.scoreLine(four,9,1)==ShapeScore::FOUR,"deduplicate open four");
    Board fork;
    for(Point p:{Point(6,7),Point(8,7),Point(7,6),Point(7,8)}) require(fork.place(p,Stone::Black),"setup");
    Evaluator eval;
    require(eval.evaluateMove(fork,Point(7,7),Stone::Black)>=ShapeScore::DOUBLE_THREE,"double three bonus");
    std::cout<<"Pattern regressions passed\n";
}
