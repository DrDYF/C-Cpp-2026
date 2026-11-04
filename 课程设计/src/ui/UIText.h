#pragma once
#include "raylib.h"

// Call after InitWindow; release before CloseWindow.
namespace UIText {
    bool load();
    void unload();
    void draw(const char* text, int x, int y, int size, Color color);
    int measure(const char* text, int size);
}
