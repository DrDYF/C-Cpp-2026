#pragma once
#include <vector>

// 落子动画
struct PlaceAnimation {
    int x, y;
    float progress;  // 0.0 -> 1.0
    bool active;
};

// 胜利高亮动画
struct WinAnimation {
    std::vector<int> highlightIndices;  // 五连棋子的索引
    float progress;
    bool active;
};

class Animator {
public:
    Animator();
    
    void update(float deltaTime);
    
    void startPlaceAnimation(int x, int y);
    void startWinAnimation(const std::vector<int>& indices);
    
    float getPlaceScale(int x, int y) const;
    float getPlaceAlpha(int x, int y) const;
    float getWinGlow(int index) const;
    
private:
    static constexpr float PLACE_DURATION = 0.12f;  // 120ms
    static constexpr float WIN_DURATION = 1.5f;
    
    PlaceAnimation placeAnim_;
    WinAnimation winAnim_;
};
