#include <iostream>
#include <limits>

using namespace std;

/*
 * 判断 n 是不是素数。
 *
 * 试除法：只要试到 sqrt(n) 就够了 —— 如果 n 有一个大于 sqrt(n) 的因子，
 * 那它必然还有一个小于 sqrt(n) 的搭档，早就被我们试出来了。
 *
 * 【注意这里写的是 i <= n / i，而不是 i * i <= n】
 * n 取到 long long 能表示的最大值时（约 9.2e18），i 会一直涨到 3e9 附近，
 * 此时 i * i 已经超过 long long 的范围，会溢出成负数。
 * 一旦溢出，循环条件就会算错：要么误判，要么变成死循环。
 * 改成除法 n / i 之后，运算结果永远不超过 n，从根上避免了平方溢出。
 *
 * 另外先把 2 和偶数挑掉，后面每次加 2，循环次数直接减半。
 */
bool isPrime(long long n) {
    if (n < 2) return false;            // 0、1 和负数都不是素数
    if (n % 2 == 0) return n == 2;      // 偶数里只有 2 是素数
    for (long long i = 3; i <= n / i; i += 2)
        if (n % i == 0) return false;
    return true;
}

// 读一个整数，读不到就返回 false（比如用户输入了字母）
bool readNumber(long long& n) {
    cout << "请输入一个正整数: ";
    if (cin >> n) return true;
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cout << "输入无效：请输入一个整数。\n";
    return false;
}

int main() {
    long long n;
    if (!readNumber(n)) return 1;

    if (n < 1) {
        cout << n << " 不是正整数。\n";
        return 1;
    }

    cout << n << (isPrime(n) ? " 是素数" : " 不是素数") << '\n';
    return 0;
}
