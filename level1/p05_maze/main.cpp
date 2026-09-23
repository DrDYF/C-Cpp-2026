#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <utility>
#include <conio.h>
#include <windows.h>
using namespace std;

//迷宫尺寸
const int MAZE_W = 31;
const int MAZE_H = 21;
const int EXIT_X = MAZE_W - 2;   // 出口坐标
const int EXIT_Y = MAZE_H - 2;

enum class Key { Up, Down, Left, Right, Quit, None };

void initTerminal() {
    SetConsoleOutputCP(CP_UTF8);// 让中文正常显示
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(h, &mode))
        SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING); // 开启 ANSI
}

Key readKey() {
    int c = _getch();
    if (c == 0 || c == 0xE0) {
        switch (_getch()) {
            case 72: return Key::Up;
            case 80: return Key::Down;
            case 75: return Key::Left;
            case 77: return Key::Right;
        }
        return Key::None;
    }
    switch (c) {
        case 'w': case 'W': return Key::Up;
        case 's': case 'S': return Key::Down;
        case 'a': case 'A': return Key::Left;
        case 'd': case 'D': return Key::Right;
        case 'q': case 'Q': return Key::Quit;
    }
    return Key::None;
}

//隐藏光标，退出时恢复
struct CursorHider {
    CursorHider()  { cout << "\033[?25l" << flush; }
    ~CursorHider() { cout << "\033[?25h" << flush; }
};

// 随机迷宫生成
vector<string> generateMaze() {
    vector<string> m(MAZE_H, string(MAZE_W, '#'));
    mt19937 rng(static_cast<unsigned>(random_device{}()));

    const int dx[4] = { 0,  0, -2,  2 };
    const int dy[4] = { -2, 2,  0,  0 };

    vector<pair<int,int>> st;
    m[1][1] = ' ';
    st.emplace_back(1, 1);

    while (!st.empty()) {
        int cx = st.back().first;
        int cy = st.back().second;

        // 收集尚未访问的邻居（距离为 2 的奇数格）
        int cand[4], n = 0;
        for (int i = 0; i < 4; ++i) {
            int nx = cx + dx[i], ny = cy + dy[i];
            if (nx > 0 && nx < MAZE_W - 1 &&
                ny > 0 && ny < MAZE_H - 1 &&
                m[ny][nx] == '#')
                cand[n++] = i;
        }

        if (n == 0) {
            st.pop_back();
        } else {
            int i  = cand[rng() % n];    // 随机选一个方向
            int nx = cx + dx[i], ny = cy + dy[i];
            m[(cy + ny) / 2][(cx + nx) / 2] = ' ';   // 打通中间的墙
            m[ny][nx] = ' ';
            st.emplace_back(nx, ny);
        }
    }
    return m;
}

//           渲染
void render(const vector<string>& m, int px, int py, int moves, bool won) {
    string s;
    s.reserve(8192);
    s += "\033[H";                       // 光标回到左上角（局部刷新，不闪烁）

    for (int y = 0; y < MAZE_H; ++y) {
        for (int x = 0; x < MAZE_W; ++x) {
            if (x == px && y == py)               s += "\033[92m@\033[0m";  // 玩家 亮绿
            else if (x == EXIT_X && y == EXIT_Y)  s += "\033[93mE\033[0m";  // 出口 亮黄
            else if (m[y][x] == '#')              s += "\033[90m#\033[0m";  // 墙   深灰
            else                                  s += ' ';                // 通路
        }
        s += "\033[K\n";                 // 清除本行尾部残留
    }

    s += "步数: " + to_string(moves) + "    操作: 方向键 / WASD 移动,  Q 退出\033[K\n";
    if (won)
        s += "\n\033[93m***胜利！***\033[0m\033[K\n";
    cout << s << flush;
}

int main() {
    initTerminal();
    CursorHider hider;    // 隐藏光标

    vector<string> maze = generateMaze();

    int px = 1, py = 1;   // 玩家起点
    int moves = 0;
    bool won  = false;

    cout << "\033[2J";               // 清屏
    render(maze, px, py, moves, false);

    while (true) {
        Key k = readKey();
        if (k == Key::Quit) break;
        if (k == Key::None) continue;

        int nx = px, ny = py;
        switch (k) {
            case Key::Up:    --ny; break;
            case Key::Down:  ++ny; break;
            case Key::Left:  --nx; break;
            case Key::Right: ++nx; break;
            default: break;
        }

        // 越界 或 撞墙 原地不动
        if (nx < 0 || nx >= MAZE_W || ny < 0 || ny >= MAZE_H) continue;
        if (maze[ny][nx] == '#') continue;

        px = nx; py = ny; ++moves;

        if (px == EXIT_X && py == EXIT_Y) {          // 到达出口
            render(maze, px, py, moves, true);
            won = true;
            break;
        }
        render(maze, px, py, moves, false);
    }

    if (!won) {
        cout << "\n已退出游戏。\n";
    } else {
        cout << "\n（按任意键结束）" << flush;
        readKey();
        cout << "\n";
    }
    return 0;
}