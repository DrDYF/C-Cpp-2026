#pragma once
#include "raylib.h"

// 主题配置：所有颜色、尺寸集中管理
namespace Theme {
    // 窗口
    constexpr int WINDOW_WIDTH = 1100;
    constexpr int WINDOW_HEIGHT = 750;
    
    // 棋盘
    constexpr int BOARD_SIZE = 15;
    constexpr int CELL_SIZE = 40;
    constexpr int BOARD_MARGIN = 50;
    constexpr int BOARD_PADDING = CELL_SIZE;
    
    // 侧边栏
    constexpr int SIDEBAR_WIDTH = 280;
    constexpr int SIDEBAR_MARGIN = 20;
    
    // 颜色
    inline Color BG_COLOR() { return Color{245, 235, 220, 255}; }
    inline Color BOARD_COLOR() { return Color{220, 179, 92, 255}; }
    inline Color LINE_COLOR() { return Color{50, 50, 50, 255}; }
    inline Color BLACK_STONE() { return Color{30, 30, 30, 255}; }
    inline Color WHITE_STONE() { return Color{245, 245, 245, 255}; }
    inline Color HOVER_COLOR() { return Color{100, 100, 100, 100}; }
    inline Color LAST_MOVE_COLOR() { return Color{220, 20, 60, 255}; }
    inline Color PANEL_BG() { return Color{255, 255, 255, 230}; }
    inline Color TEXT_COLOR() { return Color{60, 60, 60, 255}; }
    inline Color BUTTON_COLOR() { return Color{100, 149, 237, 255}; }
    inline Color BUTTON_HOVER() { return Color{70, 130, 220, 255}; }
    
    // 棋子
    constexpr float STONE_RADIUS = CELL_SIZE * 0.42f;
    
    // 计算棋盘起始位置
    inline int getBoardOffsetX() {
        int boardPixelSize = (BOARD_SIZE - 1) * CELL_SIZE + 2 * BOARD_PADDING;
        return (WINDOW_WIDTH - SIDEBAR_WIDTH - boardPixelSize) / 2;
    }
    
    inline int getBoardOffsetY() {
        int boardPixelSize = (BOARD_SIZE - 1) * CELL_SIZE + 2 * BOARD_PADDING;
        return (WINDOW_HEIGHT - boardPixelSize) / 2;
    }
    
    // 侧边栏起始位置
    inline int getSidebarX() {
        int boardPixelSize = (BOARD_SIZE - 1) * CELL_SIZE + 2 * BOARD_PADDING;
        return getBoardOffsetX() + boardPixelSize + SIDEBAR_MARGIN;
    }
    
    // 棋盘坐标 <-> 屏幕坐标
    inline Vector2 boardToScreen(int x, int y) {
        return Vector2{
            static_cast<float>(getBoardOffsetX() + BOARD_PADDING + x * CELL_SIZE),
            static_cast<float>(getBoardOffsetY() + BOARD_PADDING + y * CELL_SIZE)
        };
    }
    
    inline Point screenToBoard(Vector2 mousePos) {
        float x = (mousePos.x - getBoardOffsetX() - BOARD_PADDING + CELL_SIZE / 2.0f) / CELL_SIZE;
        float y = (mousePos.y - getBoardOffsetY() - BOARD_PADDING + CELL_SIZE / 2.0f) / CELL_SIZE;
        return Point(static_cast<int>(x), static_cast<int>(y));
    }
}
