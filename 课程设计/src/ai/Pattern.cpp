#include "Pattern.h"
#include <cstring>

PatternRecognizer::PatternRecognizer() {
    initPatterns();
}

void PatternRecognizer::initPatterns() {
    shapeTable_.fill(ShapeType::NONE);
    
    // 简化版：运行时根据字符串匹配识别
    // 完整版应该在这里预计算所有可能的 9 格编码
    // 这里先用简化实现，M4 阶段再优化为查表
}

ShapeType PatternRecognizer::recognizeLine(const int8_t* line, int8_t color) const {
    // line[4] 是中心点
    // 简化版棋型识别：扫描前后各4格
    
    int count = 0;  // 连续己方棋子数
    int leftSpace = 0, rightSpace = 0;  // 两端空格数
    int leftBlock = 0, rightBlock = 0;  // 两端是否被堵
    
    // 从中心向左扫
    int i = 4;
    while (i >= 0 && line[i] == color) {
        count++;
        i--;
    }
    int leftEnd = i;
    if (i >= 0 && line[i] == 0) {
        while (i >= 0 && line[i] == 0) {
            leftSpace++;
            i--;
        }
        if (i >= 0 && line[i] == color) leftBlock = 0;  // 跳空后还是己方
        else leftBlock = 1;
    } else {
        leftBlock = 1;
    }
    
    // 从中心向右扫
    i = 5;
    while (i < 9 && line[i] == color) {
        count++;
        i++;
    }
    int rightEnd = i;
    if (i < 9 && line[i] == 0) {
        while (i < 9 && line[i] == 0) {
            rightSpace++;
            i++;
        }
        if (i < 9 && line[i] == color) rightBlock = 0;
        else rightBlock = 1;
    } else {
        rightBlock = 1;
    }
    
    // 根据连子数和堵塞情况判断棋型
    if (count >= 5) return ShapeType::FIVE;
    
    if (count == 4) {
        if (leftBlock == 0 && rightBlock == 0) return ShapeType::FOUR;  // 活四
        if (leftBlock == 0 || rightBlock == 0) return ShapeType::BLOCKED_FOUR;  // 冲四
        return ShapeType::DEAD;
    }
    
    if (count == 3) {
        if (leftBlock == 0 && rightBlock == 0) return ShapeType::THREE;  // 活三
        if (leftBlock == 0 || rightBlock == 0) return ShapeType::SLEEP_THREE;  // 眠三
        return ShapeType::DEAD;
    }
    
    if (count == 2) {
        if (leftBlock == 0 && rightBlock == 0 && leftSpace + rightSpace >= 3) {
            return ShapeType::TWO;  // 活二
        }
        if ((leftBlock == 0 || rightBlock == 0) && leftSpace + rightSpace >= 2) {
            return ShapeType::SLEEP_TWO;  // 眠二
        }
        return ShapeType::DEAD;
    }
    
    return ShapeType::NONE;
}

int PatternRecognizer::getScore(ShapeType shape) {
    switch (shape) {
        case ShapeType::FIVE: return ShapeScore::FIVE;
        case ShapeType::FOUR: return ShapeScore::FOUR;
        case ShapeType::BLOCKED_FOUR: return ShapeScore::BLOCKED_FOUR;
        case ShapeType::THREE: return ShapeScore::THREE;
        case ShapeType::SLEEP_THREE: return ShapeScore::SLEEP_THREE;
        case ShapeType::TWO: return ShapeScore::TWO;
        case ShapeType::SLEEP_TWO: return ShapeScore::SLEEP_TWO;
        default: return 0;
    }
}
