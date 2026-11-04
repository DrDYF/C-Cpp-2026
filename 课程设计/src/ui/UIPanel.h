#pragma once
#include "raylib.h"
#include "core/Board.h"
#include "ai/Search.h"

enum class GameMode { PVE_BLACK, PVE_WHITE };

class UIPanel {
public:
    UIPanel();
    void draw(const Board& board, GameMode mode, bool aiThinking, const SearchResult& result=SearchResult{}, const char* error="", bool cancelling=false);
    bool handleInput(Vector2 mousePos, bool mousePressed);
    bool shouldUndo() { bool v = undoClicked_; undoClicked_ = false; return v; }
    bool shouldReset() { bool v = resetClicked_; resetClicked_ = false; return v; }
    bool shouldChangeMode() { bool v = modeClicked_; modeClicked_ = false; return v; }
    GameMode getSelectedMode() const { return selectedMode_; }
private:
    struct Button { Rectangle rect; const char* text; };
    void drawButton(const Button& button) const;
    void drawInfo(const Board& board, int yOffset);
    void drawModeSelection(int yOffset);
    Button undoBtn_, resetBtn_;
    bool undoClicked_ = false, resetClicked_ = false, modeClicked_ = false;
    GameMode selectedMode_ = GameMode::PVE_BLACK;
};
