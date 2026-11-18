#pragma once
#include "Board.h"
#include <fstream>
#include <iosfwd>
#include <string>
#include <vector>

// 棋谱格式 v1：纯 ASCII，一行一个事件；坐标 0..14，x 为列，y 为行。
//   gomoku-record 1
//   meta <key=value ...>
//   move <seq> <ply> <x> <y> <B|W> <human|ai> <ms> <depth> <nodes> <score> <reason> <status>
//   undo <seq> <count>
//   end <seq> <reset|mode|exit>
// seq 为事件序号，从 1 连续递增；ply 为落子后的盘面手数。
// 悔棋后同一 ply 会出现多次，因此按 seq 定位事件；按 ply 截取的是最终棋局的前 N 手。
struct RecordMove {
    Point point;
    Stone color=Stone::Empty;
    std::string source="human";
    long long elapsedMs=0;
    int depth=0;
    long long nodes=0;
    int score=0;
    std::string reason="none", status="none";
};
struct RecordEvent {
    enum class Kind { Move, Undo, End };
    Kind kind=Kind::Move;
    int seq=0, line=0, ply=0, undoCount=0;
    RecordMove move;
    std::string endReason;
};
struct GameRecord {
    int version=0;
    std::string meta;
    std::vector<RecordEvent> events;
};

bool parseRecord(std::istream& in,GameRecord& record,std::string& error);
bool loadRecord(const std::string& path,GameRecord& record,std::string& error);
// 依次应用 seq<=uptoSeq 的事件（uptoSeq<0 表示全部），并校验手数、执方与落子合法性。
bool replayRecord(const GameRecord& record,int uptoSeq,Board& board,std::string& error);

class GameRecordWriter {
public:
    // 开始新的一局；文件在第一条事件时才创建，空局不留文件。
    void begin(const std::string& directory,const std::string& meta);
    void addMove(int ply,const RecordMove& move);
    void addUndo(int count);
    // reason 为空时只关闭文件，不写 end 行。
    void finish(const char* reason);
    bool failed() const { return failed_; }
    const std::string& path() const { return path_; }
private:
    bool ensureOpen();
    void writeLine(const std::string& line);
    std::string directory_, meta_, path_;
    std::ofstream out_;
    int seq_=0;
    bool active_=false, failed_=false;
};
