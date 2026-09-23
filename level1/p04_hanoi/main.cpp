#include <iostream>
using namespace std;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    cout<<"A -> B"<<endl;
    for (int i=0;i<15;i++)cout<<"A -> C"<<endl<<"B -> C"<<endl<<"A -> B"<<endl<<"C -> B"<<endl;
    cout<<"A -> C"<<endl<<"B -> C"<<endl<<"A -> B"<<endl<<"C -> B"<<endl<<"A -> C"<<endl<<"B -> C"<<endl;
    return 0;
}