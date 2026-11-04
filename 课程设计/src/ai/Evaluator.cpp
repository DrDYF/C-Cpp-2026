#include "Evaluator.h"

Evaluator::Evaluator() {
}

int Evaluator::evaluate(const Board& board, Stone aiColor) {
    Stone oppColor = (aiColor == Stone::Black) ? Stone::White : Stone::Black;
    
    int myScore = evaluateColor(board, aiColor);
    int oppScore = evaluateColor(board, oppColor);
    
    // 稍微偏防守
    return myScore - static_cast<int>(oppScore * 1.1);
}

int Evaluator::evaluateColor(const Board& board, Stone color) {
    int totalScore = 0;
    
    // 扫描所有有子的位置
    for (int y = 0; y < BoardConst::SIZE; ++y) {
        for (int x = 0; x < BoardConst::SIZE; ++x) {
            Point p(x, y);
            if (board.at(p) != color) continue;
            
            // 四个方向
            const int dirs[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};
            
            for (auto& dir : dirs) {
                int dummy1, dummy2;
                analyzeLine(board, p, dir[0], dir[1], color, totalScore, dummy2);
            }
        }
    }
    
    return totalScore / 4;  // 每条线被计算了2次（两端），所以除以4
}

void Evaluator::analyzeLine(const Board& board, Point p, int dx, int dy, 
                            Stone color, int& myScore, int& oppScore) {
    // 提取一条线的9格（中心前后各4格）
    int8_t line[9];
    for (int i = -4; i <= 4; ++i) {
        Point pos(p.x + dx * i, p.y + dy * i);
        if (!pos.isValid()) {
            line[i + 4] = -1;  // 墙
        } else {
            Stone s = board.at(pos);
            if (s == Stone::Empty) line[i + 4] = 0;
            else if (s == color) line[i + 4] = 1;
            else line[i + 4] = 2;
        }
    }
    
    ShapeType shape = pattern_.recognizeLine(line, 1);
    myScore += PatternRecognizer::getScore(shape);
}

int Evaluator::evaluateMove(const Board& board, Point p, Stone color) {
    if (!p.isValid() || board.at(p) != Stone::Empty) return -1;
    
    int score = 0;
    
    // 四个方向
    const int dirs[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};
    
    for (auto& dir : dirs) {
        // 提取这条线
        int8_t line[9];
        for (int i = -4; i <= 4; ++i) {
            Point pos(p.x + dir[0] * i, p.y + dir[1] * i);
            if (!pos.isValid()) {
                line[i + 4] = -1;
            } else {
                Stone s = board.at(pos);
                if (s == Stone::Empty) {
                    // 假设在中心位置落子
                    line[i + 4] = (i == 0) ? 1 : 0;
                } else if (s == color) {
                    line[i + 4] = 1;
                } else {
                    line[i + 4] = 2;
                }
            }
        }
        
        ShapeType shape = pattern_.recognizeLine(line, 1);
        score += PatternRecognizer::getScore(shape);
    }
    
    return score;
}
