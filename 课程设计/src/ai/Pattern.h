#pragma once
#include <cstdint>
#include <vector>

enum class ShapeType : int { NONE, DEAD, SLEEP_TWO, TWO, SLEEP_THREE, THREE, BLOCKED_FOUR, FOUR, FIVE };
namespace ShapeScore {
constexpr int FIVE=10000000, FOUR=100000, BLOCKED_FOUR=10000, THREE=8000;
constexpr int SLEEP_THREE=1000, TWO=800, SLEEP_TWO=100, DEAD=0;
constexpr int DOUBLE_THREE=40000, FOUR_THREE=200000, DOUBLE_FOUR=300000;
}
class PatternRecognizer {
public:
    PatternRecognizer();
    ShapeType recognizeLine(const int8_t* line, int8_t color) const;
    static int getScore(ShapeType shape);
    // Full line scoring groups overlapping windows by their original stone set.
    int scoreLine(const int8_t* line, int length, int8_t color) const;
private:
    mutable std::vector<uint8_t> shapeTable_;
};
