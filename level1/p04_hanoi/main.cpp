#include <iostream>
using namespace std;
// n:    待移动的圆盘数
// from: 起始柱
// to:   目标柱
// aux:  辅助柱
void hanoi(int n, char from, char to, char aux) {
    if (n == 1) {
        cout << from << " -> " << to << '\n';
        return;
    }
    // 1. 把上面 n-1 个圆盘从 from 移到 aux（借助 to）
    hanoi(n - 1, from, aux, to);
    // 2. 把最大的圆盘从 from 移到 to
    cout << from << " -> " << to << '\n';
    // 3. 把 n-1 个圆盘从 aux 移到 to（借助 from）
    hanoi(n - 1, aux, to, from);
}

int main() {
    int n;
    cout << "请输入圆盘数量: ";
    cin >> n;
    if (n < 1) {
        cout << "圆盘数量必须为正整数。\n";
        return 1;
    }
    hanoi(n, 'A', 'C', 'B');
    return 0;
}
