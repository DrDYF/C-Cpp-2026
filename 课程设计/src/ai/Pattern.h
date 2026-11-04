#pragma once
#include <array>
#include <cstdint>

// 棋型枚举
enum class ShapeType : int {
    NONE = 0,
    DEAD = 1,
    SLEEP_TWO = 2,
    TWO = 3,
    SLEEP_THREE = 4,
    THREE = 5,
    BLOCKED_FOUR = 6,
    FOUR = 7,
    FIVE = 8
};

// 棋型分数
namespace ShapeScore {
    constexpr int FIVE = 10000000;
    constexpr int FOUR = 100000;
    constexpr int BLOCKED_FOUR = 10000;
    constexpr int THREE = 8000;
    constexpr int SLEEP_THREE = 1000;
    constexpr int TWO = 800;
    constexpr int SLEEP_TWO = 100;
    constexpr int DEAD = 0;
}

// 棋型识别器
class PatternRecognizer {
public:
    PatternRecognizer();
    
    // 识别一条线上的棋型（中心点前后各4格，共9格）
    ShapeType recognizeLine(const int8_t* line, int8_t color) const;
    
    // 获取棋型分数
    static int getScore(ShapeType shape);
    
private:
    void initPatterns();
    
    // 查表法：9格编码 -> 棋型
    std::array<ShapeType, 262144> shapeTable_;  // 4^9 = 262144
};
