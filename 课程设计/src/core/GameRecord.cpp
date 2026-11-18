#include "GameRecord.h"
#include <ctime>
#include <filesystem>
#include <sstream>
#include <system_error>

namespace {
std::string localStamp(const char* format) {
    std::time_t now=std::time(nullptr);
    std::tm local{};
    if(const std::tm* p=std::localtime(&now)) local=*p;
    char buffer[32]{};
    std::strftime(buffer,sizeof(buffer),format,&local);
    return buffer;
}
}

bool parseRecord(std::istream& in,GameRecord& record,std::string& error) {
    record=GameRecord{};
    std::string text;
    int lineNo=0, lastSeq=0;
    bool ended=false;
    auto fail=[&](const std::string& why) { error="line "+std::to_string(lineNo)+": "+why; return false; };
    while(std::getline(in,text)) {
        ++lineNo;
        if(!text.empty() && text.back()=='\r') text.pop_back();
        std::istringstream s(text);
        std::string kind;
        s>>kind;
        if(lineNo==1) {
            if(kind!="gomoku-record" || !(s>>record.version) || record.version!=1) return fail("bad header");
            continue;
        }
        if(kind.empty()) continue;
        if(kind=="meta") { std::getline(s>>std::ws,record.meta); continue; }
        if(ended) return fail("event after end");
        RecordEvent e;
        e.line=lineNo;
        if(!(s>>e.seq)) return fail("missing seq");
        if(e.seq!=lastSeq+1) return fail("seq not consecutive");
        if(kind=="move") {
            e.kind=RecordEvent::Kind::Move;
            std::string color;
            RecordMove& m=e.move;
            if(!(s>>e.ply>>m.point.x>>m.point.y>>color>>m.source>>m.elapsedMs>>m.depth>>m.nodes>>m.score>>m.reason>>m.status))
                return fail("bad move fields");
            if(color=="B") m.color=Stone::Black;
            else if(color=="W") m.color=Stone::White;
            else return fail("bad color");
        } else if(kind=="undo") {
            e.kind=RecordEvent::Kind::Undo;
            if(!(s>>e.undoCount) || e.undoCount<1) return fail("bad undo count");
        } else if(kind=="end") {
            e.kind=RecordEvent::Kind::End;
            if(!(s>>e.endReason)) return fail("missing end reason");
            ended=true;
        } else return fail("unknown event "+kind);
        std::string extra;
        if(s>>extra) return fail("trailing field "+extra);
        lastSeq=e.seq;
        record.events.push_back(e);
    }
    if(lineNo==0) { error="empty file"; return false; }
    return true;
}

bool loadRecord(const std::string& path,GameRecord& record,std::string& error) {
    std::ifstream in(path);
    if(!in) { error="cannot open "+path; return false; }
    return parseRecord(in,record,error);
}

bool replayRecord(const GameRecord& record,int uptoSeq,Board& board,std::string& error) {
    board.reset();
    for(const RecordEvent& e:record.events) {
        if(uptoSeq>=0 && e.seq>uptoSeq) break;
        auto fail=[&](const std::string& why) { error="line "+std::to_string(e.line)+": "+why; return false; };
        if(e.kind==RecordEvent::Kind::Move) {
            if(e.ply!=static_cast<int>(board.getHistory().size())+1) return fail("ply mismatch");
            if(e.move.color!=board.getCurrentPlayer()) return fail("wrong side to move");
            if(!board.place(e.move.point,e.move.color)) return fail("illegal move");
        } else if(e.kind==RecordEvent::Kind::Undo) {
            for(int i=0;i<e.undoCount;++i) if(!board.undo()) return fail("undo past start");
        }
    }
    return true;
}

void GameRecordWriter::begin(const std::string& directory,const std::string& meta) {
    if(out_.is_open()) out_.close();
    directory_=directory; meta_=meta; path_.clear();
    seq_=0; active_=true; failed_=false;
}

bool GameRecordWriter::ensureOpen() {
    if(!active_ || failed_) return false;
    if(out_.is_open()) return true;
    namespace fs=std::filesystem;
    std::error_code ec;
    fs::create_directories(fs::path(directory_),ec);
    const std::string base=directory_+"/"+localStamp("%Y%m%d-%H%M%S");
    std::string candidate;
    for(int i=0;i<1000;++i) {
        candidate=base+(i?"-"+std::to_string(i):"")+".txt";
        if(!fs::exists(fs::path(candidate),ec)) break;
    }
    out_.open(candidate,std::ios::out|std::ios::trunc);
    if(!out_) { failed_=true; return false; }
    path_=candidate;
    writeLine("gomoku-record 1");
    writeLine("meta "+(meta_.empty()?std::string():meta_+" ")+"started="+localStamp("%Y-%m-%dT%H:%M:%S"));
    return !failed_;
}

void GameRecordWriter::writeLine(const std::string& line) {
    out_<<line<<'\n';
    out_.flush();
    if(!out_) failed_=true;
}

void GameRecordWriter::addMove(int ply,const RecordMove& m) {
    if(!ensureOpen()) return;
    std::ostringstream line;
    line<<"move "<<++seq_<<' '<<ply<<' '<<m.point.x<<' '<<m.point.y<<' '<<(m.color==Stone::Black?'B':'W')<<' '
        <<m.source<<' '<<m.elapsedMs<<' '<<m.depth<<' '<<m.nodes<<' '<<m.score<<' '<<m.reason<<' '<<m.status;
    writeLine(line.str());
}

void GameRecordWriter::addUndo(int count) {
    if(count<1 || !ensureOpen()) return;
    writeLine("undo "+std::to_string(++seq_)+" "+std::to_string(count));
}

void GameRecordWriter::finish(const char* reason) {
    if(out_.is_open()) {
        if(reason && *reason && !failed_) writeLine("end "+std::to_string(++seq_)+" "+reason);
        out_.close();
    }
    active_=false;
}
