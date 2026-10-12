#include <iostream>
#include <limits>

using namespace std;

// n:     待移动的圆盘数
// from:  起始柱
// to:    目标柱
// aux:   辅助柱
// steps: 顺便数一下一共搬了多少次
void hanoi(int n, char from, char to, char aux, unsigned long long& steps) {
    if (n == 1) {
        cout << from << " -> " << to << '\n';
        ++steps;
        return;
    }
    // 1. 把上面 n-1 个圆盘从 from 移到 aux（借助 to）
    hanoi(n - 1, from, aux, to, steps);
    // 2. 把最大的圆盘从 from 移到 to
    cout << from << " -> " << to << '\n';
    ++steps;
    // 3. 把 n-1 个圆盘从 aux 移到 to（借助 from）
    hanoi(n - 1, aux, to, from, steps);
}

// 读一个正整数，读到非数字或者小于 1 的数都算失败
bool readDiskCount(int& n) {
    cout << "请输入圆盘数量: ";
    if (!(cin >> n)) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "输入无效，请输入一个整数。\n";
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

    // 盘数多了输出会刷屏，先提醒一句
    if (n > 20)
        cout << "提示：n = " << n << " 时要搬 2^n - 1 次，输出会非常长。\n";

    unsigned long long steps = 0;
    hanoi(n, 'A', 'C', 'B', steps);

    cout << "共移动 " << steps << " 次";
    if (n < 64) {                      // 2^64 已经超出 64 位整数了
        unsigned long long least = (1ULL << n) - 1;
        cout << "（理论最少 2^" << n << " - 1 = " << least << " 次）";
    }
    cout << '\n';
    return 0;
}
