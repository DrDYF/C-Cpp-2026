#include <iostream>
#include <limits>

using namespace std;

// 判断 n 是不是素数。只要试到 sqrt(n) 就够了：
// 如果 n 有大于 sqrt(n) 的因子，那它一定还有个小于 sqrt(n) 的搭档。
//
// 循环条件写成 i <= n / i，不是 i * i <= n：
// n 特别大的时候 i * i 会超出 long long 的范围，一溢出条件就不准了。
bool isPrime(long long n) {
    if (n < 2) return false;
    if (n % 2 == 0) return n == 2;      // 偶数里只有 2 是素数
    for (long long i = 3; i <= n / i; i += 2)
        if (n % i == 0) return false;
    return true;
}

// 读一个整数，读不到（比如输了个字母）就返回 false
bool readNumber(long long& n) {
    cout << "请输入一个正整数: ";
    if (cin >> n) return true;
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cout << "输入无效，请输入一个整数。\n";
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
