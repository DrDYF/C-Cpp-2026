#pragma once
#include "Types.h"
#include <cstdint>
#include <array>

class Zobrist {
public:
    static void init();
    static uint64_t hash(const class Board& board);
    
    static uint64_t getValue(int x, int y, Stone color);
    
private:
    static std::array<std::array<uint64_t, 3>, 225> table_;  // 15*15, 3 states
    static bool initialized_;
};
