#include "core/GameRecord.h"
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>

// 用法：gomoku_replay <record.txt> [--seq N | --ply N] [--position out.txt]
//   --seq N  应用到第 N 个事件（含悔棋）后的局面
//   --ply N  最终棋局的前 N 手
//   --position 输出 match_move 可读的 "x y" 逐行局面
int main(int argc,char** argv) {
    if(argc<2) { std::cerr<<"usage: gomoku_replay <record.txt> [--seq N | --ply N] [--position out.txt]\n"; return 2; }
    int seq=-1, ply=-1;
    const char* position=nullptr;
    for(int i=2;i<argc;++i) {
        if(!std::strcmp(argv[i],"--seq") && i+1<argc) seq=std::atoi(argv[++i]);
        else if(!std::strcmp(argv[i],"--ply") && i+1<argc) ply=std::atoi(argv[++i]);
        else if(!std::strcmp(argv[i],"--position") && i+1<argc) position=argv[++i];
        else { std::cerr<<"unknown argument: "<<argv[i]<<'\n'; return 2; }
    }
    if(seq>=0 && ply>=0) { std::cerr<<"use either --seq or --ply\n"; return 2; }
    GameRecord record; std::string error;
    if(!loadRecord(argv[1],record,error)) { std::cerr<<error<<'\n'; return 3; }
    Board board;
    if(!replayRecord(record,seq,board,error)) { std::cerr<<error<<'\n'; return 4; }
    if(ply>=0) {
        const auto history=board.getHistory();
        if(ply>static_cast<int>(history.size())) { std::cerr<<"final game has only "<<history.size()<<" moves\n"; return 4; }
        board.reset();
        for(int i=0;i<ply;++i) board.place(history[i],board.getCurrentPlayer());
    }
    std::cout<<"meta "<<record.meta<<'\n';
    for(const RecordEvent& e:record.events) {
        if(seq>=0 && e.seq>seq) break;
        if(e.kind==RecordEvent::Kind::Move)
            std::cout<<e.seq<<" move ply="<<e.ply<<' '<<(e.move.color==Stone::Black?'B':'W')<<" ("<<e.move.point.x<<','<<e.move.point.y<<") "
                     <<e.move.source<<' '<<e.move.elapsedMs<<"ms depth="<<e.move.depth<<" nodes="<<e.move.nodes
                     <<" score="<<e.move.score<<' '<<e.move.reason<<'/'<<e.move.status<<'\n';
        else if(e.kind==RecordEvent::Kind::Undo) std::cout<<e.seq<<" undo "<<e.undoCount<<'\n';
        else std::cout<<e.seq<<" end "<<e.endReason<<'\n';
    }
    std::cout<<"\n   ";
    for(int x=0;x<15;++x) std::cout<<(x<10?" ":"")<<x;
    std::cout<<'\n';
    const Point last=board.getLastMove();
    for(int y=0;y<15;++y) {
        std::cout<<(y<10?" ":"")<<y<<' ';
        for(int x=0;x<15;++x) {
            const Stone s=board.at(Point(x,y));
            const char c=s==Stone::Black?'X':s==Stone::White?'O':'.';
            std::cout<<(Point(x,y)==last?'[':' ')<<c;
        }
        std::cout<<'\n';
    }
    const Stone winner=board.checkWin();
    std::cout<<"ply "<<board.getHistory().size()<<", "
             <<(winner==Stone::Black?"black wins":winner==Stone::White?"white wins":
                board.getCurrentPlayer()==Stone::Black?"black to move":"white to move")<<'\n';
    if(position) {
        std::ofstream out(position);
        for(Point p:board.getHistory()) out<<p.x<<' '<<p.y<<'\n';
        if(!out) { std::cerr<<"cannot write "<<position<<'\n'; return 5; }
    }
    return 0;
}
