#include "UIPanel.h"
#include "Theme.h"
#include "UIText.h"
#include <cstdio>

UIPanel::UIPanel() {
    const float x = static_cast<float>(Theme::getSidebarX());
    undoBtn_ = {{x, 100, 120, 40}, "悔棋 (Z)"};
    resetBtn_ = {{x + 130, 100, 120, 40}, "重开 (R)"};
}

void UIPanel::draw(const Board& board, GameMode, bool aiThinking, const SearchResult& result, const char* error, bool cancelling) {
    int x = Theme::getSidebarX();
    UIText::draw("五子棋 Gomoku", x, 30, 24, Theme::TEXT_COLOR());
    drawButton(undoBtn_);
    drawButton(resetBtn_);
    drawInfo(board, 220);
    drawModeSelection(320);
    if(error && *error) UIText::draw(error,x,450,16,RED);
    else if (aiThinking) UIText::draw(cancelling ? "正在取消旧搜索..." : "AI 思考中...", x, 450, 18, Theme::TEXT_COLOR());
    if (result.nodes>0) {
        UIText::draw("上次搜索", x, 480, 16, Theme::TEXT_COLOR());
        UIText::draw(TextFormat("深度 %d / 节点 %lld",result.depth,result.nodes), x, 505, 16, DARKGRAY);
        UIText::draw(TextFormat("评分 %+d",result.score), x, 530, 16, DARKGRAY);
        UIText::draw(TextFormat("静止节点 %lld / 总 %lld",result.quiescenceNodes,result.nodes), x, 550, 12, DARKGRAY);
        const char* reason=result.reason==SearchReason::ForcedDefense ? "必堵 / 落子后静态评分" :
            result.reason==SearchReason::ImmediateWin ? "一步成五" :
            result.reason==SearchReason::MultipleThreatLoss ? "多路成五威胁 / 败势" : "";
        UIText::draw(reason, x, 570, 12, DARKGRAY);
    }
    UIText::draw("提示:\nZ - 悔棋\nR - 重开\n鼠标点击落子", x, 590, 14, DARKGRAY);
}

void UIPanel::drawButton(const Button& btn) const {
    bool hovered = CheckCollisionPointRec(GetMousePosition(), btn.rect);
    DrawRectangleRounded(btn.rect, 0.2f, 8, hovered ? Theme::BUTTON_HOVER() : Theme::BUTTON_COLOR());
    int width = UIText::measure(btn.text, 24);
    UIText::draw(btn.text, static_cast<int>(btn.rect.x + (btn.rect.width - width)/2),
                 static_cast<int>(btn.rect.y + (btn.rect.height - 24)/2), 24, WHITE);
}

void UIPanel::drawInfo(const Board& board, int y) {
    int x = Theme::getSidebarX();
    const char* player = board.getCurrentPlayer() == Stone::Black ? "黑方" : "白方";
    UIText::draw(TextFormat("当前: %s", player), x, y, 18, Theme::TEXT_COLOR());
    UIText::draw(TextFormat("步数: %d", static_cast<int>(board.getHistory().size())), x, y+30, 18, Theme::TEXT_COLOR());
    Stone winner = board.checkWin();
    if (winner != Stone::Empty)
        UIText::draw(winner == Stone::Black ? "黑方获胜！" : "白方获胜！", x, y+60, 20, RED);
    else if (board.getHistory().size() == 225)
        UIText::draw("Draw", x, y+60, 20, DARKGRAY);
}

void UIPanel::drawModeSelection(int y) {
    int x = Theme::getSidebarX();
    UIText::draw("游戏模式:", x, y, 16, Theme::TEXT_COLOR());
    const char* modes[] = {"玩家执黑", "玩家执白"};
    for (int i=0; i<2; ++i) {
        Rectangle rect{static_cast<float>(x), static_cast<float>(y+25+i*35), 250, 30};
        bool selected = selectedMode_ == static_cast<GameMode>(i);
        bool hover = CheckCollisionPointRec(GetMousePosition(), rect);
        DrawRectangleRounded(rect, 0.15f, 6, selected ? Theme::BUTTON_COLOR() : hover ? Theme::BUTTON_HOVER() : Color{230,230,230,255});
        UIText::draw(modes[i], x+10, y+30+i*35, 24, selected ? WHITE : Theme::TEXT_COLOR());
    }
}

bool UIPanel::handleInput(Vector2 pos, bool pressed) {
    if (!pressed) return false;
    if (CheckCollisionPointRec(pos, undoBtn_.rect)) { undoClicked_ = true; return true; }
    if (CheckCollisionPointRec(pos, resetBtn_.rect)) { resetClicked_ = true; return true; }
    for (int i=0; i<2; ++i) {
        Rectangle rect{static_cast<float>(Theme::getSidebarX()), static_cast<float>(345+i*35), 250, 30};
        if (CheckCollisionPointRec(pos, rect)) { selectedMode_ = static_cast<GameMode>(i); modeClicked_ = true; return true; }
    }
    return false;
}
