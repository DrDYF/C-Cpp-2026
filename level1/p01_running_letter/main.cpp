#include <iostream>
#include <windows.h>
using namespace std;

int main() {
    int s = 0;
    int dir = 1;
    while (true) {
        for (int i = 0; i < s; i++) cout << " ";
        cout << "a" << endl;
        s += dir;
        if (s >= 50 || s <= 0) dir = -dir;
        Sleep(50);
    }
    return 0;
}