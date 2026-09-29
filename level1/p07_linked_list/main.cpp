#include <iostream>

using namespace std;

// 单向链表节点
struct Node {
    int value;
    Node* next;

    Node(int v, Node* n = nullptr) : value(v), next(n) {}
};

// 往链表尾部插一个节点，这样能按输入顺序建表
void append(Node*& head, int value) {
    Node* node = new Node(value);
    if (head == nullptr) {
        head = node;
        return;
    }
    Node* tail = head;
    while (tail->next != nullptr) tail = tail->next;
    tail->next = node;
}

// 依次打印每个节点的值
void printList(const Node* head) {
    for (const Node* p = head; p != nullptr; p = p->next)
        cout << p->value << (p->next ? " -> " : "");
    cout << '\n';
}

// 就地反转整条链表（只改指针，不新建节点），返回新的头节点
Node* reverseList(Node* head) {
    Node* prev = nullptr;
    while (head != nullptr) {
        Node* next = head->next;    // 先把下一个存好，不然改完指针就找不到了
        head->next = prev;
        prev = head;
        head = next;
    }
    return prev;
}

// 从第 skip 个节点之后开始找值为 value 的节点，
// 返回它的序号（从 1 开始数），没找到返回 -1。
// skip = 0 就是从头开始找。
int findValue(const Node* head, int value, int skip) {
    int index = 0;
    for (const Node* p = head; p != nullptr; p = p->next) {
        ++index;
        if (index <= skip) continue;
        if (p->value == value) return index;
    }
    return -1;
}

void freeList(Node* head) {
    while (head != nullptr) {
        Node* next = head->next;
        delete head;
        head = next;
    }
}

int main() {
    // 建一条链表：1 2 3 5 7 5 9
    const int data[] = {1, 2, 3, 5, 7, 5, 9};
    Node* head = nullptr;
    for (int v : data) append(head, v);

    cout << "原链表: ";
    printList(head);

    head = reverseList(head);
    cout << "反转后: ";
    printList(head);

    int first = findValue(head, 5, 0);
    if (first == -1)
        cout << "链表里没有 5\n";
    else
        cout << "第一个 5 在第 " << first << " 个节点\n";

    // 从刚才那个节点后面接着找
    if (first != -1) {
        int next = findValue(head, 5, first);
        if (next == -1)
            cout << "后面没有别的 5 了\n";
        else
            cout << "下一个 5 在第 " << next << " 个节点\n";
    }

    freeList(head);
    return 0;
}
