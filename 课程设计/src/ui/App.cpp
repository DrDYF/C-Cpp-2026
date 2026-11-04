#include "App.h"
#include "Theme.h"

App::App() 
    : hoverPos_(-1, -1)
    , gameMode_(GameMode::PVP)
    , aiThinking_(false)
    , running_(true) {
    
    InitWindow(Theme::WINDOW_WIDTH, Theme::WINDOW_HEIGHT, "五子棋 Gomoku");
    SetTargetFPS(60);
}

App::~App() {
    CloseWindow();
}

void App::run() {
    while (running_ && !WindowShouldClose()) {
        processInput();
        update();
        render();
    }
}

void App::processInput() {
    Vector2 mousePos = GetMousePosition();
    bool mousePressed = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    
    // UI 面板处理
    if (panel_.handleInput(mousePos, mousePressed)) {
        if (panel_.shouldUndo()) {
            board_.undo();
            if (gameMode_ != GameMode::PVP) {
                board_.undo();
            }
        }
        if (panel_.shouldReset()) {
            board_.reset();
            aiThinking_ = false;
        }
        if (panel_.shouldChangeMode()) {
            gameMode_ = panel_.getSelectedMode();
            board_.reset();
            aiThinking_ = false;
        }
        return;
    }
    
    // 棋盘交互
    hoverPos_ = Theme::screenToBoard(mousePos);
    
    if (mousePressed && hoverPos_.isValid() && board_.at(hoverPos_) == Stone::Empty) {
        handlePlayerMove(hoverPos_);
    }
    
    // 快捷键
    if (IsKeyPressed(KEY_Z)) {
        board_.undo();
        if (gameMode_ != GameMode::PVP) {
            board_.undo();
        }
    }
    
    if (IsKeyPressed(KEY_R)) {
        board_.reset();
        aiThinking_ = false;
    }
}

void App::handlePlayerMove(Point p) {
    Stone currentPlayer = board_.getCurrentPlayer();
    
    // 检查模式限制
    if (gameMode_ == GameMode::PVE_BLACK && currentPlayer == Stone::White) return;
    if (gameMode_ == GameMode::PVE_WHITE && currentPlayer == Stone::Black) return;
    
    if (board_.place(p, currentPlayer)) {
        // 触发 AI 思考
        if (gameMode_ != GameMode::PVP && board_.checkWin() == Stone::Empty) {
            aiThinking_ = true;
        }
    }
}

void App::update() {
    gameMode_ = panel_.getSelectedMode();
    
    // AI 思考
    if (aiThinking_ && board_.checkWin() == Stone::Empty) {
        handleAIMove();
        aiThinking_ = false;
    }
}

void App::handleAIMove() {
    Stone currentPlayer = board_.getCurrentPlayer();
    SearchResult result = aiEngine_.think(board_, 1500);
    
    if (result.move.isValid()) {
        board_.place(result.move, currentPlayer);
    }
}

void App::render() {
    BeginDrawing();
    ClearBackground(Theme::BG_COLOR());
    
    renderer_.draw(board_, hoverPos_);
    panel_.draw(board_, gameMode_, aiThinking_);
    
    EndDrawing();
}
