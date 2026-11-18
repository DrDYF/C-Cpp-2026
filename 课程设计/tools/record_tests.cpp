#include "core/GameRecord.h"
#include <filesystem>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace fs=std::filesystem;
namespace {
void require(bool ok,const std::string& why) { if(!ok) throw std::runtime_error(why); }
RecordMove mv(int x,int y,Stone color,const char* source="human") {
    RecordMove m; m.point=Point(x,y); m.color=color; m.source=source; return m;
}
bool parses(const std::string& text,GameRecord& record,std::string& error) {
    std::istringstream in(text);
    return parseRecord(in,record,error);
}
size_t fileCount(const fs::path& dir) {
    if(!fs::exists(dir)) return 0;
    size_t n=0;
    for(const auto& entry:fs::directory_iterator(dir)) if(entry.is_regular_file()) ++n;
    return n;
}
}

int main() {
    try {
        const fs::path dir=fs::temp_directory_path()/"gomoku-record-tests";
        std::error_code ec;
        fs::remove_all(dir,ec);

        GameRecordWriter empty;
        empty.begin(dir.string(),"mode=test");
        empty.finish("reset");
        require(empty.path().empty() && fileCount(dir)==0,"empty game must not create a file");

        GameRecordWriter w;
        w.begin(dir.string(),"mode=test budget_ms=100");
        w.addMove(1,mv(7,7,Stone::Black));
        RecordMove ai=mv(8,8,Stone::White,"ai");
        ai.elapsedMs=97; ai.depth=5; ai.nodes=1234; ai.score=-40; ai.reason="normal"; ai.status="move";
        w.addMove(2,ai);
        w.addMove(3,mv(7,8,Stone::Black));
        w.addUndo(2);
        w.addMove(2,mv(6,6,Stone::White));
        w.addMove(3,mv(8,8,Stone::Black));
        w.finish("reset");
        require(!w.failed() && !w.path().empty(),"writer failed");

        GameRecord rec; std::string err;
        require(loadRecord(w.path(),rec,err),"load: "+err);
        require(rec.events.size()==7,"event count");
        require(rec.meta.find("mode=test budget_ms=100")==0 && rec.meta.find("started=")!=std::string::npos,"meta");
        const RecordMove& back=rec.events[1].move;
        require(back.source=="ai" && back.elapsedMs==97 && back.depth==5 && back.nodes==1234 && back.score==-40 &&
                back.reason=="normal" && back.status=="move" && back.point==Point(8,8) && back.color==Stone::White,"move fields roundtrip");
        require(rec.events[3].kind==RecordEvent::Kind::Undo && rec.events[3].undoCount==2,"undo event");
        require(rec.events[6].kind==RecordEvent::Kind::End && rec.events[6].endReason=="reset","end event");

        Board direct;
        direct.place(Point(7,7),Stone::Black); direct.place(Point(6,6),Stone::White); direct.place(Point(8,8),Stone::Black);
        Board replay;
        require(replayRecord(rec,-1,replay,err),"replay all: "+err);
        require(replay.getHistory().size()==3 && replay.getHash()==direct.getHash(),"final position hash");
        require(replayRecord(rec,3,replay,err) && replay.getHistory().size()==3 && replay.at(Point(7,8))==Stone::Black,"replay to seq 3");
        require(replayRecord(rec,4,replay,err) && replay.getHistory().size()==1,"replay to undo");

        GameRecordWriter second;
        second.begin(dir.string(),"mode=test");
        second.addMove(1,mv(7,7,Stone::Black));
        second.finish("exit");
        require(!second.path().empty() && second.path()!=w.path() && fileCount(dir)==2,"same-second games need distinct files");

        const std::string head="gomoku-record 1\nmeta x=1\n";
        GameRecord bad;
        require(!parses("gomoku-record 2\n",bad,err),"version check");
        require(!parses(head+"move 1 1 7 7 B human 0 0 0 0 none none\nmove 3 2 8 8 W human 0 0 0 0 none none\n",bad,err) &&
                err.find("line 4")==0,"seq gap reports line");
        require(!parses(head+"move 1 1 7 7 X human 0 0 0 0 none none\n",bad,err),"bad color");
        require(!parses(head+"end 1 reset\nundo 2 1\n",bad,err),"event after end");
        require(parses(head+"move 1 1 7 7 B human 0 0 0 0 none none\nmove 2 2 8 8 B human 0 0 0 0 none none\n",bad,err) &&
                !replayRecord(bad,-1,replay,err) && err.find("wrong side")!=std::string::npos,"wrong side");
        require(parses(head+"move 1 2 7 7 B human 0 0 0 0 none none\n",bad,err) && !replayRecord(bad,-1,replay,err),"ply mismatch");
        require(parses(head+"move 1 1 7 7 B human 0 0 0 0 none none\nundo 2 2\n",bad,err) && !replayRecord(bad,-1,replay,err),"undo past start");
        require(parses(head+"move 1 1 7 7 B human 0 0 0 0 none none\nmove 2 2 7 7 W human 0 0 0 0 none none\n",bad,err) &&
                !replayRecord(bad,-1,replay,err),"occupied point");

        fs::remove_all(dir,ec);
        std::cout<<"Record write, parse, undo/reset distinction and replay regressions passed\n";
        return 0;
    } catch(const std::exception& e) {
        std::cerr<<e.what()<<'\n';
        return 1;
    }
}
