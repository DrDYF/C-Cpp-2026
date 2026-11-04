#include "Zobrist.h"
#include "Board.h"
#include <random>
#include <mutex>

std::array<std::array<uint64_t, 3>, 225> Zobrist::table_;
bool Zobrist::initialized_ = false;

void Zobrist::init() {
    static std::once_flag once;
    std::call_once(once, [] {
        std::mt19937_64 rng(20261028);
        for (auto& square : table_) for (auto& value : square) value = rng();
    });
}

uint64_t Zobrist::hash(const Board& board) {
    init();
    
    uint64_t h = 0;
    for (int y = 0; y < BoardConst::SIZE; ++y) {
        for (int x = 0; x < BoardConst::SIZE; ++x) {
            Point p(x, y);
            Stone s = board.at(p);
            if (s != Stone::Empty) {
                h ^= getValue(x, y, s);
            }
        }
    }
    return h;
}

uint64_t Zobrist::getValue(int x, int y, Stone color) {
    init();
    int index = y * BoardConst::SIZE + x;
    return table_[index][static_cast<int>(color)];
}
