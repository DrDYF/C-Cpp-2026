#include "Animator.h"
#include <cmath>
#include <algorithm>

Animator::Animator() {
    placeAnim_.active = false;
    winAnim_.active = false;
}

void Animator::update(float deltaTime) {
    if (placeAnim_.active) {
        placeAnim_.progress += deltaTime / PLACE_DURATION;
        if (placeAnim_.progress >= 1.0f) {
            placeAnim_.progress = 1.0f;
            placeAnim_.active = false;
        }
    }
    
    if (winAnim_.active) {
        winAnim_.progress += deltaTime / WIN_DURATION;
        if (winAnim_.progress >= 1.0f) {
            winAnim_.progress = 1.0f;
        }
    }
}

void Animator::startPlaceAnimation(int x, int y) {
    placeAnim_.x = x;
    placeAnim_.y = y;
    placeAnim_.progress = 0.0f;
    placeAnim_.active = true;
}

void Animator::startWinAnimation(const std::vector<int>& indices) {
    winAnim_.highlightIndices = indices;
    winAnim_.progress = 0.0f;
    winAnim_.active = true;
}

float Animator::getPlaceScale(int x, int y) const {
    if (!placeAnim_.active || placeAnim_.x != x || placeAnim_.y != y) {
        return 1.0f;
    }
    
    // 1.3 -> 1.0 缓动
    float t = placeAnim_.progress;
    float scale = 1.3f - 0.3f * t * t;  // easeOut
    return scale;
}

float Animator::getPlaceAlpha(int x, int y) const {
    if (!placeAnim_.active || placeAnim_.x != x || placeAnim_.y != y) {
        return 1.0f;
    }
    
    return placeAnim_.progress;
}

float Animator::getWinGlow(int index) const {
    if (!winAnim_.active) return 0.0f;
    
    auto it = std::find(winAnim_.highlightIndices.begin(), 
                       winAnim_.highlightIndices.end(), index);
    if (it == winAnim_.highlightIndices.end()) return 0.0f;
    
    // 呼吸效果
    float breath = 0.5f + 0.5f * std::sin(winAnim_.progress * 6.28f * 2);
    return breath;
}
