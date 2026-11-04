#include "App.h"
#include "Theme.h"
#include "UIText.h"
#include <chrono>
#include <stdexcept>

App::App() {
    InitWindow(Theme::WINDOW_WIDTH,Theme::WINDOW_HEIGHT,"五子棋 Gomoku");
    if(!UIText::load()) { CloseWindow(); throw std::runtime_error("Chinese UI font not found"); }
    SetTargetFPS(60);
}
App::~App() {
    invalidateSearch();
    if(pending_.valid()) pending_.wait();
    UIText::unload();
    CloseWindow();
}
void App::run() {
    while(running_ && !WindowShouldClose()) { processInput(); update(); render(); }
}
bool App::isAITurn() const {
    if(board_.checkWin()!=Stone::Empty || board_.getHistory().size()==225) return false;
    return (gameMode_==GameMode::PVE_BLACK && board_.getCurrentPlayer()==Stone::White) ||
           (gameMode_==GameMode::PVE_WHITE && board_.getCurrentPlayer()==Stone::Black);
}
void App::invalidateSearch() {
    ++revision_;
    if(cancel_) cancel_->store(true);
    lastSearch_=SearchResult{};
    aiError_.clear();
    failedRevision_=~uint64_t{0};
}
void App::resetGame() {
    invalidateSearch();
    board_.reset();
    hoverPos_=Point{};
}
void App::undoTurn() {
    // Keep the AI's opening stone when the human is white and has not moved yet.
    const size_t opening=gameMode_==GameMode::PVE_WHITE ? 1 : 0;
    if(board_.getHistory().size()<=opening) return;
    invalidateSearch();
    board_.undo();
    if(isAITurn() && board_.getHistory().size()>opening) board_.undo();
}
void App::processInput() {
    Vector2 pos=GetMousePosition();
    bool pressed=IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    bool consumed=panel_.handleInput(pos,pressed);
    if(panel_.shouldChangeMode()) { gameMode_=panel_.getSelectedMode(); resetGame(); }
    if(panel_.shouldReset()) resetGame();
    if(panel_.shouldUndo()) undoTurn();
    if(IsKeyPressed(KEY_Z)) undoTurn();
    if(IsKeyPressed(KEY_R)) resetGame();
    hoverPos_=Theme::screenToBoard(pos);
    if(!consumed && pressed) handlePlayerMove(hoverPos_);
}
void App::handlePlayerMove(Point p) {
    if(isAITurn() || board_.checkWin()!=Stone::Empty) return;
    if(board_.place(p,board_.getCurrentPlayer())) ++revision_;
}
void App::update() {
    if(pending_.valid() && pending_.wait_for(std::chrono::milliseconds(0))==std::future_status::ready) {
        try {
            SearchResult result=pending_.get();
            if(searchRevision_==revision_ && isAITurn() && !cancel_->load()) {
                if(result.status==SearchStatus::Move && result.move.isValid() &&
                   board_.place(result.move,board_.getCurrentPlayer())) { lastSearch_=result; ++revision_; }
                else {
                    failedRevision_=revision_;
                    aiError_="AI 异常 / 请重开";
                    TraceLog(LOG_ERROR,"AI returned no valid move for an active board (status %i)",static_cast<int>(result.status));
                }
            }
        } catch(const std::exception& e) {
            TraceLog(LOG_ERROR,"AI worker failed: %s",e.what());
            failedRevision_=revision_;
            aiError_="AI 异常 / 请重开";
        }
    }
    if(!pending_.valid() && isAITurn() && running_ && failedRevision_!=revision_) handleAIMove();
    aiThinking_=pending_.valid();
    aiCancelling_=aiThinking_ && (searchRevision_!=revision_ || !isAITurn() || (cancel_ && cancel_->load()));
}
void App::handleAIMove() {
    Board snapshot=board_;
    cancel_=std::make_shared<std::atomic<bool>>(false);
    auto token=cancel_;
    searchRevision_=revision_;
    pending_=std::async(std::launch::async,[this,snapshot,token] {
        return aiEngine_.think(snapshot,1500,token.get());
    });
}
void App::render() {
    BeginDrawing();
    renderer_.draw(board_,isAITurn() || board_.checkWin()!=Stone::Empty ? Point{} : hoverPos_,
                   gameMode_ == GameMode::PVE_BLACK ? Stone::Black : Stone::White);
    panel_.draw(board_,gameMode_,aiThinking_,lastSearch_,aiError_.c_str(),aiCancelling_);
    EndDrawing();
}
