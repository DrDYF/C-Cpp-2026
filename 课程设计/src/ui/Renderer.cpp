#include "Renderer.h"
#include "Theme.h"
#include <cmath>

void Renderer::draw(const Board& board, Point hoverPos) {
    ClearBackground(Theme::BG_COLOR());
    
    // 更新动画
    animator_.update(GetFrameTime());
    
    drawBoard();
    drawStones(board);
    
    // 悬停预览
    if (hoverPos.isValid() && board.at(hoverPos) == Stone::Empty) {
        drawHover(hoverPos, board.getCurrentPlayer());
    }
    
    // 最后几步标记（渐变淡出）
    const auto& history = board.getHistory();
    int count = std::min(5, static_cast<int>(history.size()));
    for (int i = 0; i < count; ++i) {
        int idx = history.size() - 1 - i;
        Point p = history[idx];
        float alpha = 1.0f - i * 0.15f;
        drawLastMoveMarker(p, alpha);
    }
    
    // 胜利高亮
    Stone winner = board.checkWin();
    if (winner != Stone::Empty) {
        drawWinHighlight(board);
    }
}

void Renderer::drawBoard() {
    using namespace Theme;
    
    int offsetX = getBoardOffsetX();
    int offsetY = getBoardOffsetY();
    int boardPixelSize = (BOARD_SIZE - 1) * CELL_SIZE + 2 * BOARD_PADDING;
    
    // 棋盘底色 + 内阴影效果
    DrawRectangle(offsetX, offsetY, boardPixelSize, boardPixelSize, BOARD_COLOR());
    
    // 内阴影（四边）
    Color shadowColor = Color{0, 0, 0, 30};
    DrawRectangleGradientEx(
        Rectangle{static_cast<float>(offsetX), static_cast<float>(offsetY), 
                 static_cast<float>(boardPixelSize), 10},
        shadowColor, shadowColor, Color{0,0,0,0}, Color{0,0,0,0}
    );
    
    // 网格线
    for (int i = 0; i < BOARD_SIZE; ++i) {
        Vector2 start = boardToScreen(i, 0);
        Vector2 end = boardToScreen(i, BOARD_SIZE - 1);
        DrawLineEx(start, end, 2.0f, LINE_COLOR());
        
        start = boardToScreen(0, i);
        end = boardToScreen(BOARD_SIZE - 1, i);
        DrawLineEx(start, end, 2.0f, LINE_COLOR());
    }
    
    // 星位
    const int starPoints[][2] = {{7, 7}, {3, 3}, {3, 11}, {11, 3}, {11, 11}};
    for (auto& sp : starPoints) {
        Vector2 pos = boardToScreen(sp[0], sp[1]);
        DrawCircleV(pos, 5.0f, LINE_COLOR());
    }
    
    // 坐标标记
    for (int i = 0; i < BOARD_SIZE; ++i) {
        char label[2];
        label[0] = 'A' + i;
        label[1] = '\0';
        Vector2 pos = boardToScreen(i, 0);
        DrawText(label, pos.x - 4, offsetY - 20, 14, DARKGRAY);
        
        // 数字
        const char* num = TextFormat("%d", i + 1);
        pos = boardToScreen(0, i);
        DrawText(num, offsetX - 25, pos.y - 7, 14, DARKGRAY);
    }
}

void Renderer::drawStones(const Board& board) {
    for (int y = 0; y < BoardConst::SIZE; ++y) {
        for (int x = 0; x < BoardConst::SIZE; ++x) {
            Point p(x, y);
            Stone stone = board.at(p);
            if (stone != Stone::Empty) {
                Vector2 pos = Theme::boardToScreen(x, y);
                
                float scale = animator_.getPlaceScale(x, y);
                float alpha = animator_.getPlaceAlpha(x, y);
                
                drawStone(pos, stone, alpha, scale);
            }
        }
    }
}

void Renderer::drawHover(Point p, Stone currentPlayer) {
    Vector2 pos = Theme::boardToScreen(p.x, p.y);
    drawStone(pos, currentPlayer, 0.4f, 1.0f);
}

void Renderer::drawLastMoveMarker(Point p, float alpha) {
    Vector2 pos = Theme::boardToScreen(p.x, p.y);
    Color markerColor = Theme::LAST_MOVE_COLOR();
    markerColor.a = static_cast<unsigned char>(255 * alpha);
    DrawCircleV(pos, 6.0f, markerColor);
}

void Renderer::drawWinHighlight(const Board& board) {
    // 找出五连的棋子
    Point last = board.getLastMove();
    if (!last.isValid()) return;
    
    Stone winner = board.at(last);
    const int dirs[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};
    
    for (auto& dir : dirs) {
        std::vector<Point> line;
        line.push_back(last);
        
        // 正向
        for (int i = 1; i < 5; ++i) {
            Point next(last.x + dir[0] * i, last.y + dir[1] * i);
            if (!next.isValid() || board.at(next) != winner) break;
            line.push_back(next);
        }
        
        // 反向
        for (int i = 1; i < 5; ++i) {
            Point next(last.x - dir[0] * i, last.y - dir[1] * i);
            if (!next.isValid() || board.at(next) != winner) break;
            line.insert(line.begin(), next);
        }
        
        if (line.size() >= 5) {
            // 画连线
            for (size_t i = 0; i < line.size() - 1; ++i) {
                Vector2 start = Theme::boardToScreen(line[i].x, line[i].y);
                Vector2 end = Theme::boardToScreen(line[i+1].x, line[i+1].y);
                DrawLineEx(start, end, 4.0f, Theme::LAST_MOVE_COLOR());
            }
            
            // 发光效果
            for (auto& p : line) {
                Vector2 pos = Theme::boardToScreen(p.x, p.y);
                DrawCircleV(pos, Theme::STONE_RADIUS + 3, Color{255, 215, 0, 100});
            }
            break;
        }
    }
}

void Renderer::drawStone(Vector2 pos, Stone color, float alpha, float scale) {
    float radius = Theme::STONE_RADIUS * scale;
    
    Color stoneColor = (color == Stone::Black) ? Theme::BLACK_STONE() : Theme::WHITE_STONE();
    stoneColor.a = static_cast<unsigned char>(255 * alpha);
    
    // 径向渐变模拟（用多层圆）
    if (color == Stone::Black) {
        // 黑子：中心亮一点
        DrawCircleV(pos, radius, stoneColor);
        Vector2 highlight = {pos.x - radius * 0.3f, pos.y - radius * 0.3f};
        DrawCircleV(highlight, radius * 0.2f, Color{80, 80, 80, static_cast<unsigned char>(150 * alpha)});
    } else {
        // 白子：渐变 + 边框
        DrawCircleV(pos, radius, stoneColor);
        Color borderColor = Theme::LINE_COLOR();
        borderColor.a = static_cast<unsigned char>(150 * alpha);
        DrawCircleLines(static_cast<int>(pos.x), static_cast<int>(pos.y), radius, borderColor);
        
        // 高光
        Vector2 highlight = {pos.x - radius * 0.3f, pos.y - radius * 0.3f};
        DrawCircleV(highlight, radius * 0.25f, Color{255, 255, 255, static_cast<unsigned char>(180 * alpha)});
    }
    
    // 投影（仅完全显示时）
    if (alpha >= 0.99f && scale >= 0.99f) {
        Vector2 shadowPos = {pos.x + 2, pos.y + 2};
        DrawCircleV(shadowPos, radius, Color{0, 0, 0, 30});
    }
}
