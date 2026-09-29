#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <ctime>
#include <conio.h>
#include <windows.h>

using namespace std;

/*
 * 推箱子
 *
 * 地图文件里的符号：
 *   #    墙            (空格) 地板
 *   @    玩家          +      玩家站在目标点上
 *   $    箱子          *      箱子已经被推到目标点上
 *   .    目标点
 *
 * 把所有箱子都推到目标点就算过关，走的步数越少越好。
 */

// ---------- 关卡数据 ----------

struct Level {
    vector<string> ground;      // 静态层：'#' 墙，' ' 地板，'.' 目标点
    vector<string> things;      // 动态层：' ' 空，'$' 箱子
    int px = 0, py = 0;         // 玩家坐标（列 x，行 y）
    int width = 0, height = 0;
    int steps = 0;              // 本关已经走了多少步
};

// ---------- 终端相关 ----------

enum class Key { Up, Down, Left, Right, Restart, Quit, None };

void initTerminal() {
    SetConsoleOutputCP(CP_UTF8);        // 不然中文会变成乱码
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(h, &mode))
        SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);   // 打开 ANSI 转义
}

Key readKey() {
    int c = _getch();
    if (c == 0 || c == 0xE0) {          // 方向键会先发一个前缀，再发真正的键码
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
        case 'r': case 'R': return Key::Restart;
        case 'q': case 'Q': return Key::Quit;
    }
    return Key::None;
}

// 进游戏把光标藏起来，退出时再放出来，画面干净一点
struct CursorHider {
    CursorHider()  { cout << "\033[?25l" << flush; }
    ~CursorHider() { cout << "\033[?25h" << flush; }
};

void clearScreen() { cout << "\033[2J\033[H" << flush; }

// ---------- 找文件 ----------

// CLion 默认在 build 目录里跑程序，当前目录不一定是源码目录。
// 这里挨个试几个可能的位置，哪个能读到关卡文件就用哪个。
string findGameDir() {
    string here = __FILE__;
    size_t slash = here.find_last_of("\\/");
    string srcDir = (slash == string::npos) ? "." : here.substr(0, slash);

    const vector<string> candidates = {
        srcDir,
        ".",
        "level1/p06_push_boxes",
        "../level1/p06_push_boxes",
        "../../level1/p06_push_boxes",
    };
    for (const string& dir : candidates) {
        ifstream probe(dir + "/levels/level1.txt");
        if (probe) return dir;
    }
    return srcDir;
}

int countLevels(const string& gameDir) {
    int n = 0;
    while (true) {
        ifstream probe(gameDir + "/levels/level" + to_string(n + 1) + ".txt");
        if (!probe) break;
        ++n;
    }
    return n;
}

string nowString() {
    time_t t = time(nullptr);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&t));
    return buf;
}

// ---------- 关卡读写 ----------

bool loadLevel(const string& path, Level& lv) {
    ifstream fin(path);
    if (!fin) return false;

    vector<string> lines;
    string line;
    while (getline(fin, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();   // 兼容 Windows 换行
        lines.push_back(line);
    }
    if (lines.empty()) return false;

    lv.height = (int)lines.size();
    lv.width = 0;
    for (const string& s : lines) lv.width = max(lv.width, (int)s.size());

    lv.ground.assign(lv.height, string(lv.width, ' '));
    lv.things.assign(lv.height, string(lv.width, ' '));
    lv.steps = 0;

    for (int y = 0; y < lv.height; ++y) {
        for (int x = 0; x < lv.width; ++x) {
            char c = (x < (int)lines[y].size()) ? lines[y][x] : ' ';
            switch (c) {
                case '#': lv.ground[y][x] = '#'; break;
                case '.': lv.ground[y][x] = '.'; break;
                case '$': lv.things[y][x] = '$'; break;
                case '*': lv.ground[y][x] = '.'; lv.things[y][x] = '$'; break;
                case '@': lv.px = x; lv.py = y; break;
                case '+': lv.ground[y][x] = '.'; lv.px = x; lv.py = y; break;
                default:  break;      // 其它字符一律当普通地板
            }
        }
    }
    return true;
}

// ---------- 游戏逻辑 ----------

// 所有箱子都在目标点上就算过关
bool isCleared(const Level& lv) {
    for (int y = 0; y < lv.height; ++y)
        for (int x = 0; x < lv.width; ++x)
            if (lv.things[y][x] == '$' && lv.ground[y][x] != '.')
                return false;
    return true;
}

// 试着往 (dx, dy) 走一步。撞墙或者箱子推不动就原地不动。
void tryMove(Level& lv, int dx, int dy) {
    int nx = lv.px + dx, ny = lv.py + dy;
    if (nx < 0 || nx >= lv.width || ny < 0 || ny >= lv.height) return;
    if (lv.ground[ny][nx] == '#') return;

    if (lv.things[ny][nx] == '$') {          // 前面有个箱子，看看能不能推
        int bx = nx + dx, by = ny + dy;
        if (bx < 0 || bx >= lv.width || by < 0 || by >= lv.height) return;
        if (lv.ground[by][bx] == '#') return;
        if (lv.things[by][bx] == '$') return;   // 两个箱子叠一起可不行
        lv.things[ny][nx] = ' ';
        lv.things[by][bx] = '$';
    }

    lv.px = nx;
    lv.py = ny;
    ++lv.steps;
}

// ---------- 渲染 ----------

void render(const Level& lv, int levelNo, int levelCount, bool cleared) {
    string out;
    out.reserve(4096);
    out += "\033[H";

    for (int y = 0; y < lv.height; ++y) {
        for (int x = 0; x < lv.width; ++x) {
            bool onGoal = (lv.ground[y][x] == '.');
            if (x == lv.px && y == lv.py)
                out += onGoal ? "\033[92m+\033[0m" : "\033[92m@\033[0m";   // 玩家 亮绿
            else if (lv.things[y][x] == '$')
                out += onGoal ? "\033[93m*\033[0m" : "\033[93m$\033[0m";   // 箱子 亮黄
            else if (lv.ground[y][x] == '#')
                out += "\033[90m#\033[0m";                                  // 墙   深灰
            else if (onGoal)
                out += "\033[93m.\033[0m";                                  // 目标点
            else
                out += ' ';
        }
        out += "\033[K\n";
    }

    out += "第 " + to_string(levelNo) + "/" + to_string(levelCount) + " 关"
         + "    步数: " + to_string(lv.steps)
         + "    方向键/WASD 移动,  R 重来,  Q 退出\033[K\n";

    if (cleared)
        out += "\n\033[93m*** 过关！按任意键继续 ***\033[0m\033[K\n";

    cout << out << flush;
}

// ---------- 主流程 ----------

int main() {
    initTerminal();
    CursorHider hider;

    string gameDir = findGameDir();
    int levelCount = countLevels(gameDir);
    if (levelCount == 0) {
        cout << "没找到关卡文件，看看 " << gameDir << "/levels/ 里有没有 level1.txt\n";
        return 1;
    }

    // 每跑一次就追加一段记录，方便回头对比成绩
    string scoreFile = gameDir + "/scores.txt";
    ofstream score(scoreFile, ios::app);
    if (score) score << "==== " << nowString() << " ====\n";

    int totalSteps = 0;
    bool quit = false;

    for (int no = 1; no <= levelCount && !quit; ++no) {
        string path = gameDir + "/levels/level" + to_string(no) + ".txt";
        Level lv;
        if (!loadLevel(path, lv)) break;

        while (true) {
            clearScreen();
            render(lv, no, levelCount, false);

            Key k = readKey();
            if (k == Key::Quit) { quit = true; break; }
            if (k == Key::Restart) { loadLevel(path, lv); continue; }

            int dx = 0, dy = 0;
            if (k == Key::Up)         dy = -1;
            else if (k == Key::Down)  dy = 1;
            else if (k == Key::Left)  dx = -1;
            else if (k == Key::Right) dx = 1;
            else continue;                  // 其它按键直接忽略

            tryMove(lv, dx, dy);

            if (isCleared(lv)) {            // 箱子全归位了
                clearScreen();
                render(lv, no, levelCount, true);
                readKey();                  // 等玩家按一下再走
                break;
            }
        }

        if (!quit) {
            totalSteps += lv.steps;
            cout << "第 " << no << " 关用了 " << lv.steps << " 步\n";
            if (score) score << "第 " << no << " 关  步数 " << lv.steps << '\n';
        }
    }

    if (!quit) {
        cout << "\n全部通关！总共 " << totalSteps << " 步\n";
        if (score) score << "全部通关，总步数 " << totalSteps << "\n";
    }
    cout << "成绩记录在 " << scoreFile << '\n';

    return 0;
}
