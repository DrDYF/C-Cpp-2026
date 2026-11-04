#pragma once
#include "Types.h"
#include <array>
#include <vector>

class Board {
public:
    Board();
    
    // 落子，返回是否成功
    bool place(Point p, Stone color);
    
    // 悔棋
    bool undo();
    
    // 重新开始
    void reset();
    
    // 获取指定位置的棋子
    Stone at(Point p) const;
    
    // 判断胜负（返回获胜方，Empty 表示未分出胜负）
    Stone checkWin() const;
    
    // 获取最后一步
    Point getLastMove() const { return lastMove_; }
    
    // 获取历史记录
    const std::vector<Point>& getHistory() const { return history_; }
    
    // 当前该谁下
    uint64_t getHash() const { return hash_; }
    Stone getCurrentPlayer() const;
    
private:
    // 内部索引转换（带 padding）
    inline int idx(int x, int y) const {
        return (y + BoardConst::PADDING) * BoardConst::STRIDE + (x + BoardConst::PADDING);
    }
    
    // 检查从某点开始的某个方向是否有五连
    bool checkLine(Point p, int dx, int dy, Stone color) const;
    
    std::array<int8_t, BoardConst::STRIDE * BoardConst::STRIDE> cells_;
    std::vector<Point> history_;
    Point lastMove_;
    Stone winner_;
    uint64_t hash_ = 0;
};
