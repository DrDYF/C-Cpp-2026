#include <iostream>
#include <vector>
#include <chrono>
#include <cmath>
using namespace std;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    const int N = 1000;

    auto start = chrono::high_resolution_clock::now();

    vector<bool> isPrime(N + 1, true);
    isPrime[0] = isPrime[1] = false;

    for (int i = 2; i * i <= N; ++i) {
        if (isPrime[i]) {
            for (int j = i * i; j <= N; j += i) {
                isPrime[j] = false;
            }
        }
    }

    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = end - start;

    for (int i = 2; i <= N; ++i) {
        if (isPrime[i]) {
            cout << i << '\n';
        }
    }

    cout <<"时间："<< elapsed.count() << " 秒\n";

    return 0;
}