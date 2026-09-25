#include <iostream>
#include <string>
#include <conio.h>
#include <windows.h>

using namespace std;

int main() {
    const string word = "Hello";    // 要奔跑的字母/单词
    const int delay = 60;           // 每帧间隔(毫秒)，越小跑得越快

    // 拿控制台窗口宽度当左右边界；取不到（比如输出被重定向了）就按 80 列算
    int width = 80;
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info))
        width = info.srWindow.Right - info.srWindow.Left + 1;
    if (width < (int)word.size() + 2) width = (int)word.size() + 2;   // 别窄到放不下

    int pos = 0;    // 单词左边的空格数
    int step = 1;   // +1 向右，-1 向左

    while (!_kbhit()) {             // 按任意键就退出
        // 用 \r 回到行首重画，后面补空格是为了擦掉上一帧留下的字
        int pad = width - pos - (int)word.size();
        cout << '\r' << string(pos, ' ') << word
             << string(pad > 0 ? pad : 0, ' ') << flush;

        // 先按当前方向走，撞到边界就掉头
        if (pos + (int)word.size() >= width) step = -1;
        if (pos <= 0) step = 1;
        pos += step;

        Sleep(delay);
    }

    _getch();
    cout << '\n';
    return 0;
}
