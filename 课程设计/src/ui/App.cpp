#include "App.h"
#include "Theme.h"
#include "UIText.h"
#include "ai/SearchText.h"
#include <chrono>
#include <stdexcept>

App::App() {
    InitWindow(Theme::WINDOW_WIDTH,Theme::WINDOW_HEIGHT,"五子棋 Gomoku");
    if(!UIText::load()) { CloseWindow(); throw std::runtime_error("Chinese UI font not found"); }
    SetTargetFPS(60);
    startRecord();
}
App::~App() {
    invalidateSearch();
    if(pending_.valid()) pending_.wait();
    record_.finish("exit");
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
void App::startRecord() {
    const bool humanBlack=gameMode_==GameMode::PVE_BLACK;
    record_.begin(std::string(GetApplicationDirectory())+"records",
                  std::string("mode=")+(humanBlack?"pve_black":"pve_white")+" human="+(humanBlack?"B":"W")+
                  " budget_ms="+std::to_string(AI_BUDGET_MS));
}
void App::resetGame(const char* recordEnd) {
    invalidateSearch();
    board_.reset();
    hoverPos_=Point{};
    record_.finish(recordEnd);
    startRecord();
}
void App::undoTurn() {
    // Keep the AI's opening stone when the human is white and has not moved yet.
    const size_t opening=gameMode_==GameMode::PVE_WHITE ? 1 : 0;
    if(board_.getHistory().size()<=opening) return;
    invalidateSearch();
    int undone=board_.undo()?1:0;
    if(isAITurn() && board_.getHistory().size()>opening && board_.undo()) ++undone;
    record_.addUndo(undone);
}
void App::processInput() {
    Vector2 pos=GetMousePosition();
    bool pressed=IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    bool consumed=panel_.handleInput(pos,pressed);
    if(panel_.shouldChangeMode()) { gameMode_=panel_.getSelectedMode(); resetGame("mode"); }
    if(panel_.shouldReset()) resetGame();
    if(panel_.shouldUndo()) undoTurn();
    if(IsKeyPressed(KEY_Z)) undoTurn();
    if(IsKeyPressed(KEY_R)) resetGame();
    hoverPos_=Theme::screenToBoard(pos);
    if(!consumed && pressed) handlePlayerMove(hoverPos_);
}
void App::handlePlayerMove(Point p) {
    if(isAITurn() || board_.checkWin()!=Stone::Empty) return;
    const Stone color=board_.getCurrentPlayer();
    if(!board_.place(p,color)) return;
    ++revision_;
    RecordMove move;
    move.point=p; move.color=color; move.source="human";
    record_.addMove(static_cast<int>(board_.getHistory().size()),move);
}
void App::update() {
    if(pending_.valid() && pending_.wait_for(std::chrono::milliseconds(0))==std::future_status::ready) {
        try {
            SearchResult result=pending_.get();
            if(searchRevision_==revision_ && isAITurn() && !cancel_->load()) {
                const Stone color=board_.getCurrentPlayer();
                if(result.status==SearchStatus::Move && result.move.isValid() && board_.place(result.move,color)) {
                    lastSearch_=result; ++revision_;
                    RecordMove move;
                    move.point=result.move; move.color=color; move.source="ai";
                    move.elapsedMs=result.elapsedMs; move.depth=result.depth; move.nodes=result.nodes; move.score=result.score;
                    move.reason=searchReasonName(result.reason); move.status=searchStatusName(result.status);
                    record_.addMove(static_cast<int>(board_.getHistory().size()),move);
                }
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
        const auto start=std::chrono::steady_clock::now();
        SearchResult result=aiEngine_.think(snapshot,AI_BUDGET_MS,token.get());
        result.elapsedMs=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count();
        return result;
    });
}
void App::render() {
    BeginDrawing();
    renderer_.draw(board_,isAITurn() || board_.checkWin()!=Stone::Empty ? Point{} : hoverPos_,
                   gameMode_ == GameMode::PVE_BLACK ? Stone::Black : Stone::White);
    panel_.draw(board_,gameMode_,aiThinking_,lastSearch_,aiError_.c_str(),aiCancelling_);
    if(record_.failed()) UIText::draw("棋谱未保存",Theme::getSidebarX(),598,12,RED);
    EndDrawing();
}
