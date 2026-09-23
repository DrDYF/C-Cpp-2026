#include <iostream>
using namespace std;

int main() {
    int n;
    bool isPrime = true;
    cin>>n;
    if(n<=1){
        isPrime = false;
    }
    for(int i=2;i<=sqrt(n);i++){
        if(n%i==0){
            isPrime = false;
            break;
        }
    }
    if(isPrime){
        cout << "是素数" << endl;
    }else{
        cout << "不是素数" << endl;
    }
    return 0;
}
