#pragma once
#include "raylib.h"
#include "core/Board.h"
#include "Animator.h"

class Renderer {
public:
    Renderer() = default;
    
    void draw(const Board& board, Point hoverPos);
    
private:
    void drawBoard();
    void drawStones(const Board& board);
    void drawHover(Point p, Stone currentPlayer);
    void drawLastMoveMarker(Point p, float alpha = 1.0f);
    void drawWinHighlight(const Board& board);
    
    void drawStone(Vector2 pos, Stone color, float alpha = 1.0f, float scale = 1.0f);
    
    Animator animator_;
};
