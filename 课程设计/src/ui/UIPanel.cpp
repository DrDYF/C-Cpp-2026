#include "UIPanel.h"
#include "Theme.h"
#include <cstdio>

UIPanel::UIPanel() 
    : undoClicked_(false)
    , resetClicked_(false)
    , modeClicked_(false)
    , selectedMode_(GameMode::PVP) {
    
    int sidebarX = Theme::getSidebarX();
    
    undoBtn_ = {Rectangle{static_cast<float>(sidebarX), 100, 120, 40}, "悔棋 (Z)", false};
    resetBtn_ = {Rectangle{static_cast<float>(sidebarX + 130), 100, 120, 40}, "重开 (R)", false};
    modeBtn_ = {Rectangle{static_cast<float>(sidebarX), 160, 250, 40}, "切换模式", false};
}

void UIPanel::draw(const Board& board, GameMode mode, bool aiThinking) {
    int sidebarX = Theme::getSidebarX();
    
    // 标题
    DrawText("五子棋 Gomoku", sidebarX, 30, 24, Theme::TEXT_COLOR());
    
    // 按钮
    Vector2 mousePos = GetMousePosition();
    bool mousePressed = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    
    drawButton(undoBtn_, mousePos, mousePressed);
    drawButton(resetBtn_, mousePos, mousePressed);
    drawButton(modeBtn_, mousePos, mousePressed);
    
    // 游戏信息
    drawInfo(board, 220);
    
    // 模式选择
    drawModeSelection(320, mousePos, mousePressed);
    
    // AI 思考提示
    if (aiThinking) {
        DrawText("AI 思考中...", sidebarX, 450, 18, Theme::TEXT_COLOR());
    }
    
    // 操作提示
    const char* hint = "提示:\n"
                       "Z - 悔棋\n"
                       "R - 重开\n"
                       "鼠标点击落子";
    DrawText(hint, sidebarX, 500, 14, DARKGRAY);
}

bool UIPanel::drawButton(Button& btn, Vector2 mousePos, bool mousePressed) {
    btn.hovered = CheckCollisionPointRec(mousePos, btn.rect);
    
    Color color = btn.hovered ? Theme::BUTTON_HOVER() : Theme::BUTTON_COLOR();
    DrawRectangleRounded(btn.rect, 0.2f, 8, color);
    
    int textWidth = MeasureText(btn.text, 18);
    int textX = btn.rect.x + (btn.rect.width - textWidth) / 2;
    int textY = btn.rect.y + (btn.rect.height - 18) / 2;
    DrawText(btn.text, textX, textY, 18, WHITE);
    
    if (btn.hovered && mousePressed) {
        if (&btn == &undoBtn_) undoClicked_ = true;
        if (&btn == &resetBtn_) resetClicked_ = true;
        if (&btn == &modeBtn_) modeClicked_ = true;
        return true;
    }
    
    return false;
}

void UIPanel::drawInfo(const Board& board, int yOffset) {
    int sidebarX = Theme::getSidebarX();
    
    // 当前回合
    Stone currentPlayer = board.getCurrentPlayer();
    const char* playerName = (currentPlayer == Stone::Black) ? "黑方" : "白方";
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "当前: %s", playerName);
    DrawText(buffer, sidebarX, yOffset, 18, Theme::TEXT_COLOR());
    
    // 步数
    snprintf(buffer, sizeof(buffer), "步数: %d", static_cast<int>(board.getHistory().size()));
    DrawText(buffer, sidebarX, yOffset + 30, 18, Theme::TEXT_COLOR());
    
    // 胜负
    Stone winner = board.checkWin();
    if (winner != Stone::Empty) {
        const char* winnerName = (winner == Stone::Black) ? "黑方获胜！" : "白方获胜！";
        DrawText(winnerName, sidebarX, yOffset + 60, 20, RED);
    }
}

void UIPanel::drawModeSelection(int yOffset, Vector2 mousePos, bool mousePressed) {
    int sidebarX = Theme::getSidebarX();
    
    DrawText("游戏模式:", sidebarX, yOffset, 16, Theme::TEXT_COLOR());
    
    const char* modes[] = {"人人对战", "人机(黑)", "人机(白)"};
    
    for (int i = 0; i < 3; ++i) {
        Rectangle rect{static_cast<float>(sidebarX), static_cast<float>(yOffset + 25 + i * 35), 250, 30};
        bool selected = (selectedMode_ == static_cast<GameMode>(i));
        bool hovered = CheckCollisionPointRec(mousePos, rect);
        
        Color bgColor = selected ? Theme::BUTTON_COLOR() : 
                       hovered ? Theme::BUTTON_HOVER() : 
                       Color{230, 230, 230, 255};
        
        DrawRectangleRounded(rect, 0.15f, 6, bgColor);
        DrawText(modes[i], sidebarX + 10, yOffset + 30 + i * 35, 16, 
                selected ? WHITE : Theme::TEXT_COLOR());
        
        if (hovered && mousePressed) {
            selectedMode_ = static_cast<GameMode>(i);
        }
    }
}

bool UIPanel::handleInput(Vector2 mousePos, bool mousePressed) {
    return drawButton(undoBtn_, mousePos, mousePressed) ||
           drawButton(resetBtn_, mousePos, mousePressed) ||
           drawButton(modeBtn_, mousePos, mousePressed);
}
