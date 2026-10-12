#include <iostream>
#include <limits>

using namespace std;

/*
 * 汉诺塔：把 n 个圆盘从 from 柱整体搬到 to 柱，aux 是中转柱。
 *
 * 递归三步：
 *   1. 先把上面 n-1 个圆盘从 from 搬到 aux（借助 to）；
 *   2. 再把最底下那个最大的圆盘从 from 搬到 to；
 *   3. 最后把 aux 上的 n-1 个圆盘搬到 to（借助 from）。
 *
 * n == 1 是递归出口：一个圆盘直接搬过去。
 *
 * steps 用来统计一共搬了多少次（引用传出去给 main 用）。
 * 用 unsigned long long 是因为 64 个圆盘要搬 2^64 - 1 次，int 根本装不下。
 */
void hanoi(int n, char from, char to, char aux, unsigned long long& steps) {
    if (n == 1) {
        cout << from << " -> " << to << '\n';
        ++steps;
        return;
    }
    hanoi(n - 1, from, aux, to, steps);
    cout << from << " -> " << to << '\n';
    ++steps;
    hanoi(n - 1, aux, to, from, steps);
}

// 读一个正整数。读到非数字或者小于 1 的数都算失败。
bool readDiskCount(int& n) {
    cout << "请输入圆盘数量: ";
    if (!(cin >> n)) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "输入无效：请输入一个整数。\n";
        return false;
    }
    if (n < 1) {
        cout << "圆盘数量必须为正整数。\n";
        return false;
    }
    return true;
}

int main() {
    int n;
    if (!readDiskCount(n)) return 1;

    // n 太大时输出会长到看不完（64 盘按传说要 2^64-1 步），先提醒一句
    if (n > 20)
        cout << "提示：n = " << n << " 时最少要搬 2^n - 1 次，输出会非常长。\n";

    unsigned long long steps = 0;
    hanoi(n, 'A', 'C', 'B', steps);

    // 理论最少步数是 2^n - 1，顺便核对一下上面的递归有没有多搬
    cout << "共移动 " << steps << " 次";
    if (n < 64) {                       // n >= 64 时 2^n 已经超出 64 位整数范围
        unsigned long long least = (1ULL << n) - 1;
        cout << "（理论最少 2^" << n << " - 1 = " << least << " 次）";
    }
    cout << '\n';
    return 0;
}
