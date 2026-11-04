#pragma once
#include "raylib.h"
#include "core/Board.h"
#include "ai/AIEngine.h"
#include "Renderer.h"
#include "UIPanel.h"
#include <atomic>
#include <future>
#include <memory>
#include <string>

class App {
public:
    App();
    ~App();
    void run();
private:
    friend struct AppTestAccess;
    void processInput();
    void update();
    void render();
    bool isAITurn() const;
    void handlePlayerMove(Point p);
    void handleAIMove();
    void invalidateSearch();
    void resetGame();
    void undoTurn();
    Board board_;
    Renderer renderer_;
    UIPanel panel_;
    AIEngine aiEngine_;
    Point hoverPos_;
    GameMode gameMode_=GameMode::PVE_BLACK;
    bool aiThinking_=false;
    bool aiCancelling_=false;
    bool running_=true;
    std::future<SearchResult> pending_;
    std::shared_ptr<std::atomic<bool>> cancel_;
    uint64_t revision_=0, searchRevision_=0;
    uint64_t failedRevision_=~uint64_t{0};
    std::string aiError_;
    SearchResult lastSearch_;
};
