#include <iostream>

using namespace std;

// 判断 n 是不是素数。只要试到 sqrt(n) 就够了：
// 如果 n 有大于 sqrt(n) 的因子，那它一定还有个小于 sqrt(n) 的搭档。
bool isPrime(long long n) {
    if (n < 2) return false;
    for (long long i = 2; i * i <= n; ++i)
        if (n % i == 0) return false;
    return true;
}

int main() {
    long long n;
    cout << "请输入一个正整数: ";
    if (!(cin >> n)) {
        cout << "输入无效\n";
        return 1;
    }

    cout << n << (isPrime(n) ? " 是素数" : " 不是素数") << '\n';
    return 0;
}
