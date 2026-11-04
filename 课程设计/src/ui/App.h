#pragma once
#include "raylib.h"
#include "core/Board.h"
#include "ai/AIEngine.h"
#include "Renderer.h"
#include "UIPanel.h"

class App {
public:
    App();
    ~App();
    
    void run();
    
private:
    void processInput();
    void update();
    void render();
    
    void handlePlayerMove(Point p);
    void handleAIMove();
    
    Board board_;
    Renderer renderer_;
    UIPanel panel_;
    AIEngine aiEngine_;
    
    Point hoverPos_;
    GameMode gameMode_;
    bool aiThinking_;
    bool running_;
};
