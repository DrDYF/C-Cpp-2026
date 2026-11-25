#include "ui/App.h"
#include "ui/Theme.h"
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>

struct AppTestAccess {
    static void require(bool ok,const char* why) { if(!ok) throw std::runtime_error(why); }
    static void pump(App& app) { app.update(); app.render(); }
    static void waitFor(App& app,size_t count,int ms=2800) {
        auto end=std::chrono::steady_clock::now()+std::chrono::milliseconds(ms);
        while(app.board_.getHistory().size()!=count && std::chrono::steady_clock::now()<end) {
            pump(app); std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        require(app.board_.getHistory().size()==count,"AI did not reach expected ply count");
    }
    static void select(App& app,GameMode mode) {
        int row=mode==GameMode::PVE_BLACK?0:1;
        require(app.panel_.handleInput({static_cast<float>(Theme::getSidebarX()+20),static_cast<float>(355+row*35)},true),"mode click consumed");
        app.processInput();
    }
    static void run(App& app) {
        int x=Theme::getSidebarX();
        // A render pass must never create a second click event.
        require(app.panel_.handleInput({static_cast<float>(x+20),110},true),"undo consumed");
        app.render(); app.render();
        require(app.panel_.shouldUndo(),"undo event missing");
        require(!app.panel_.shouldUndo(),"duplicate undo event");
        app.render(); require(!app.panel_.shouldUndo(),"draw created undo event");
        app.handlePlayerMove(Point(7,7));
        require(app.board_.getHistory().size()==1,"human black opening");
        select(app,GameMode::PVE_WHITE);
        require(app.board_.getHistory().empty(),"mode change must reset");
        waitFor(app,1);
        require(app.board_.at(Point(7,7))==Stone::Black && !app.isAITurn(),"AI must open black");
        app.undoTurn(); require(app.board_.getHistory().size()==1,"keep sole AI opening");
        app.handlePlayerMove(Point(8,7));
        app.update();
        require(app.pending_.valid(),"AI worker not launched");
        auto start=std::chrono::steady_clock::now();
        long long updateMicros=0;
        for(int i=0;i<20;++i) {
            auto tick=std::chrono::steady_clock::now();
            app.update();
            updateMicros+=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-tick).count();
            app.render();
        }
        auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count();
        require(updateMicros<200000,"AI blocked main-thread updates");
        std::cout<<"20 UI frames during AI: "<<elapsed<<"ms; update CPU total="<<updateMicros<<"us\n";
        waitFor(app,3);
        require(app.panel_.handleInput({static_cast<float>(x+20),110},true),"undo round clicked");
        app.processInput();
        require(app.board_.getHistory().size()==1,"single click must undo one round");
        pump(app); pump(app);
        require(app.board_.getHistory().size()==1,"undo repeated on next frame");
        // Switch during a new search; its result must not survive the revision change.
        app.handlePlayerMove(Point(8,7)); app.update();
        select(app,GameMode::PVE_BLACK);
        require(app.board_.getHistory().empty(),"switch resets pending game");
        auto end=std::chrono::steady_clock::now()+std::chrono::seconds(2);
        while(app.pending_.valid() && std::chrono::steady_clock::now()<end) { pump(app); std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
        require(!app.pending_.valid() && app.board_.getHistory().empty(),"obsolete AI result applied");
        app.handlePlayerMove(Point(7,7)); app.update();
        app.resetGame();
        end=std::chrono::steady_clock::now()+std::chrono::seconds(2);
        while(app.pending_.valid() && std::chrono::steady_clock::now()<end) { pump(app); std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
        require(app.board_.getHistory().empty(),"reset allowed old result");
        app.handlePlayerMove(Point(7,7)); waitFor(app,2);
        require(app.lastSearch_.nodes>0 && app.lastSearch_.depth>0,"missing search statistics");
        pump(app);
        RenderTexture2D preview=LoadRenderTexture(Theme::WINDOW_WIDTH,Theme::WINDOW_HEIGHT);
        BeginTextureMode(preview);
        app.renderer_.draw(app.board_,Point{});
        app.panel_.draw(app.board_,app.gameMode_,false,app.lastSearch_);
        EndTextureMode();
        Image image=LoadImageFromTexture(preview.texture);
        ImageFlipVertical(&image);
        require(ExportImage(image,"ui-preview.png"),"export UI preview");
        UnloadImage(image); UnloadRenderTexture(preview);
        app.resetGame(); app.handlePlayerMove(Point(7,7));
        app.searchRevision_=app.revision_;
        app.cancel_=std::make_shared<std::atomic<bool>>(false);
        std::promise<SearchResult> oldWorker; app.pending_=oldWorker.get_future();
        app.resetGame(); app.update();
        require(app.aiThinking_ && app.aiCancelling_,"pending canceled worker appeared idle");
        SearchResult canceledResult; canceledResult.status=SearchStatus::Cancelled;
        oldWorker.set_value(canceledResult); app.update();
        require(!app.aiThinking_ && !app.aiCancelling_,"collected worker remained busy");
        app.handlePlayerMove(Point(7,7));
        app.searchRevision_=app.revision_;
        app.cancel_=std::make_shared<std::atomic<bool>>(false);
        std::promise<SearchResult> failure;
        app.pending_=failure.get_future();
        SearchResult noMove; noMove.status=SearchStatus::NoLegalMove; failure.set_value(noMove);
        app.update();
        require(!app.pending_.valid() && !app.aiError_.empty(),"missing move must pause with an error");
        for(int i=0;i<20;++i) app.update();
        require(!app.pending_.valid(),"failed result retried each frame");
        app.resetGame();
        require(app.aiError_.empty(),"reset must clear failure");
        app.handlePlayerMove(Point(7,7)); waitFor(app,2);
        {
            GameRecord rec; std::string err; Board replay;
            require(!app.record_.failed() && !app.record_.path().empty(),"record file missing");
            require(loadRecord(app.record_.path(),rec,err) && replayRecord(rec,-1,replay,err),"record replay failed");
            require(replay.getHash()==app.board_.getHash() && replay.getHistory().size()==app.board_.getHistory().size(),"record replay differs from board");
            require(rec.events.size()==2 && rec.events[1].move.source=="ai" && rec.events[1].move.depth>0 && rec.events[1].move.elapsedMs>0,"AI move metadata missing");
        }
        std::cout<<"UI input, opening, cancellation, failure recovery and undo regressions passed\n";
    }
    static void runShortBudget() {
        // 交互回归不需要耗尽正式比赛的思考预算。
        App app(1500);
        run(app);
    }
    static void checkReleaseBudget(const App& app) {
        require(app.aiBudgetMs_==60000,"release AI budget must be 60 seconds");
    }
};
int main() {
    for(int i=0;i<5;++i) {
        SetConfigFlags(FLAG_WINDOW_HIDDEN);
        if(i==4) AppTestAccess::runShortBudget();
        else {
            App app;
            AppTestAccess::checkReleaseBudget(app);
        }
        std::cout<<"Debug start "<<i+1<<" passed\n";
    }
}
