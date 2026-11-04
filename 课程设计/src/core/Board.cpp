#include "Board.h"
#include "Zobrist.h"

Board::Board() : lastMove_(-1, -1), winner_(Stone::Empty) {
    reset();
}

void Board::reset() {
    // 初始化为空，外围两圈填 -1（墙）
    cells_.fill(-1);
    for (int y = 0; y < BoardConst::SIZE; ++y) {
        for (int x = 0; x < BoardConst::SIZE; ++x) {
            cells_[idx(x, y)] = static_cast<int8_t>(Stone::Empty);
        }
    }
    history_.clear();
    hash_ = 0;
    lastMove_ = Point(-1, -1);
    winner_ = Stone::Empty;
}

bool Board::place(Point p, Stone color) {
    if (!p.isValid()) return false;
    if (color == Stone::Empty) return false;
    if (at(p) != Stone::Empty) return false;
    if (winner_ != Stone::Empty) return false;  // 已经分出胜负
    
    cells_[idx(p.x, p.y)] = static_cast<int8_t>(color);
    hash_ ^= Zobrist::getValue(p.x, p.y, color);
    history_.push_back(p);
    lastMove_ = p;
    
    // 检查是否获胜
    winner_ = checkWin();
    
    return true;
}

bool Board::undo() {
    if (history_.empty()) return false;
    
    Point p = history_.back();
    history_.pop_back();
    hash_ ^= Zobrist::getValue(p.x, p.y, at(p));
    cells_[idx(p.x, p.y)] = static_cast<int8_t>(Stone::Empty);
    
    lastMove_ = history_.empty() ? Point(-1, -1) : history_.back();
    winner_ = checkWin();
    
    return true;
}

Stone Board::at(Point p) const {
    if (!p.isValid()) return Stone::Empty;
    return static_cast<Stone>(cells_[idx(p.x, p.y)]);
}

Stone Board::getCurrentPlayer() const {
    return (history_.size() % 2 == 0) ? Stone::Black : Stone::White;
}

Stone Board::checkWin() const {
    if (history_.empty()) return Stone::Empty;
    
    Point last = lastMove_;
    Stone color = at(last);
    if (color == Stone::Empty) return Stone::Empty;
    
    // 四个方向：横、竖、主对角线、副对角线
    const int dirs[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};
    
    for (auto& dir : dirs) {
        if (checkLine(last, dir[0], dir[1], color)) {
            return color;
        }
    }
    
    return Stone::Empty;
}

bool Board::checkLine(Point p, int dx, int dy, Stone color) const {
    int count = 1;  // 中心点自己算一个
    
    // 正向延伸
    for (int i = 1; i < 5; ++i) {
        Point next(p.x + dx * i, p.y + dy * i);
        if (!next.isValid() || at(next) != color) break;
        count++;
    }
    
    // 反向延伸
    for (int i = 1; i < 5; ++i) {
        Point next(p.x - dx * i, p.y - dy * i);
        if (!next.isValid() || at(next) != color) break;
        count++;
    }
    
    return count >= 5;
}
