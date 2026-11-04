#pragma once
#include <cstdint>

// 棋子颜色
enum class Stone : int8_t {
    Empty = 0,
    Black = 1,
    White = 2
};

// 棋盘坐标
struct Point {
    int x, y;
    
    Point() : x(-1), y(-1) {}
    Point(int x_, int y_) : x(x_), y(y_) {}
    
    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }
    
    bool operator!=(const Point& other) const {
        return !(*this == other);
    }
    
    bool isValid() const {
        return x >= 0 && x < 15 && y >= 0 && y < 15;
    }
};

// 棋盘常量
namespace BoardConst {
    constexpr int SIZE = 15;           // 棋盘大小
    constexpr int STRIDE = SIZE + 4;   // 带 padding 的数组宽度（19）
    constexpr int PADDING = 2;         // 外围填充
}
