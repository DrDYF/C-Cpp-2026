#pragma once
#include "raylib.h"
#include "core/Board.h"

// 游戏模式
enum class GameMode {
    PVP,        // 人人对战
    PVE_BLACK,  // 玩家执黑 vs AI
    PVE_WHITE   // 玩家执白 vs AI
};

// UI 面板：按钮、信息显示
class UIPanel {
public:
    UIPanel();
    
    void draw(const Board& board, GameMode mode, bool aiThinking);
    bool handleInput(Vector2 mousePos, bool mousePressed);
    
    // 按钮事件查询（调用后自动清除）
    bool shouldUndo() { bool ret = undoClicked_; undoClicked_ = false; return ret; }
    bool shouldReset() { bool ret = resetClicked_; resetClicked_ = false; return ret; }
    bool shouldChangeMode() { bool ret = modeClicked_; modeClicked_ = false; return ret; }
    
    GameMode getSelectedMode() const { return selectedMode_; }
    
private:
    struct Button {
        Rectangle rect;
        const char* text;
        bool hovered;
    };
    
    bool drawButton(Button& btn, Vector2 mousePos, bool mousePressed);
    void drawInfo(const Board& board, int yOffset);
    void drawModeSelection(int yOffset, Vector2 mousePos, bool mousePressed);
    
    Button undoBtn_;
    Button resetBtn_;
    Button modeBtn_;
    
    bool undoClicked_;
    bool resetClicked_;
    bool modeClicked_;
    
    GameMode selectedMode_;
};
