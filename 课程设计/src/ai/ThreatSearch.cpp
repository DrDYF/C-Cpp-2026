#include "ThreatSearch.h"
#include "Pattern.h"

ThreatSearcher::ThreatSearcher(Evaluator& evaluator)
    : evaluator_(evaluator) {
}

bool ThreatSearcher::findVCF(Board& board, Stone color, int maxDepth, Point& result) {
    auto threats = generateThreats(board, color);
    
    for (auto& threat : threats) {
        board.place(threat, color);
        
        if (board.checkWin() == color) {
            result = threat;
            board.undo();
            return true;
        }
        
        if (vcfSearch(board, color, 1, maxDepth)) {
            result = threat;
            board.undo();
            return true;
        }
        
        board.undo();
    }
    
    return false;
}

bool ThreatSearcher::vcfSearch(Board& board, Stone attacker, int depth, int maxDepth) {
    if (depth >= maxDepth) return false;
    
    Stone defender = (attacker == Stone::Black) ? Stone::White : Stone::Black;
    
    // 防守方尝试所有防御
    auto defenses = generateDefenses(board, defender);
    
    if (defenses.empty()) return true;  // 无法防守
    
    for (auto& defense : defenses) {
        board.place(defense, defender);
        
        // 进攻方继续进攻
        auto threats = generateThreats(board, attacker);
        bool canWin = false;
        
        for (auto& threat : threats) {
            board.place(threat, attacker);
            
            if (board.checkWin() == attacker) {
                board.undo();
                board.undo();
                return true;
            }
            
            if (vcfSearch(board, attacker, depth + 1, maxDepth)) {
                canWin = true;
                board.undo();
                break;
            }
            
            board.undo();
        }
        
        board.undo();
        
        if (!canWin) return false;  // 找到一个有效防御
    }
    
    return true;
}

std::vector<Point> ThreatSearcher::generateThreats(const Board& board, Stone color) {
    std::vector<Point> threats;
    
    // 找出所有能形成冲四的点
    for (int y = 0; y < BoardConst::SIZE; ++y) {
        for (int x = 0; x < BoardConst::SIZE; ++x) {
            Point p(x, y);
            if (board.at(p) != Stone::Empty) continue;
            
            int score = evaluator_.evaluateMove(board, p, color);
            if (score >= ShapeScore::BLOCKED_FOUR) {
                threats.push_back(p);
            }
        }
    }
    
    return threats;
}

std::vector<Point> ThreatSearcher::generateDefenses(const Board& board, Stone color) {
    Stone attacker = (color == Stone::Black) ? Stone::White : Stone::Black;
    
    // 找出对方的威胁点附近
    std::vector<Point> defenses;
    
    for (int y = 0; y < BoardConst::SIZE; ++y) {
        for (int x = 0; x < BoardConst::SIZE; ++x) {
            Point p(x, y);
            if (board.at(p) != Stone::Empty) continue;
            
            int score = evaluator_.evaluateMove(board, p, attacker);
            if (score >= ShapeScore::THREE) {
                defenses.push_back(p);
            }
        }
    }
    
    return defenses;
}
