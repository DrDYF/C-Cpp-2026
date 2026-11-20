#pragma once
#include "Types.h"
#include <array>
#include <vector>
#if defined(_MSC_VER) && !defined(__clang__)
#include <intrin.h>
#endif

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
    
    // Incrementally maintained empty points within the configured radius of a stone.
    // The empty board exposes the center point as its opening candidate.
    void appendCandidates(std::vector<Point>& out) const;
    // 行优先遍历；访问期间不得修改棋盘。
    template<class Visitor>
    void forEachCandidate(Visitor&& visit) const {
        for(std::size_t word=0;word<candidateBits_.size();++word) {
            for(uint64_t bits=candidateBits_[word];bits;bits&=bits-1) {
                const int id=static_cast<int>(word*64)+candidateLowBit(bits);
                visit(Point(id%BoardConst::SIZE,id/BoardConst::SIZE));
            }
        }
    }
    bool isCandidate(Point p) const;
    uint8_t nearbyStoneCount(Point p) const;

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
    inline int cellId(int x,int y) const { return y*BoardConst::SIZE+x; }
    void updateCandidate(Point p);
    void setCandidate(int id,bool enabled);
    static int candidateLowBit(uint64_t bits) {
#if defined(__GNUC__) || defined(__clang__)
        return __builtin_ctzll(bits);
#elif defined(_MSC_VER) && (defined(_M_X64) || defined(_M_ARM64))
        unsigned long bit;
        _BitScanForward64(&bit,bits);
        return static_cast<int>(bit);
#else
        int bit=0;
        for(;!(bits&1);bits>>=1) ++bit;
        return bit;
#endif
    }
    bool candidateBit(int id) const { return (candidateBits_[id>>6]>>(id&63))&1ULL; }
    
    // 检查从某点开始的某个方向是否有五连
    bool checkLine(Point p, int dx, int dy, Stone color) const;
    
    std::array<int8_t, BoardConst::STRIDE * BoardConst::STRIDE> cells_;
    std::array<uint8_t,BoardConst::SIZE*BoardConst::SIZE> neighborCounts_{};
    std::array<uint64_t,4> candidateBits_{};
    std::vector<Point> history_;
    Point lastMove_;
    Stone winner_;
    uint64_t hash_ = 0;
};
