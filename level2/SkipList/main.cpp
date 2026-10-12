#include <iostream>
#include <random>
#include <string>

using namespace std;

/*
 * 跳表（SkipList）
 *
 * 在有序链表上再叠几层索引：level 0 是完整链表，往上每层节点少一半，
 * 查找从最高层往前跳，跳不动就降一层，平均 O(log n)。
 *
 * 新节点几层是掷硬币定的，每次跑出来的结构都不一样。
 */
template <typename K, typename V>
class SkipList {
public:
    static constexpr int MAX_LEVEL = 16;    // 一共最多 16 层，够放几万个元素了

    // seed 不传就取随机数；传固定值的话每次跑出来的结构一样，方便对照
    explicit SkipList(unsigned seed = random_device{}()) : rng(seed) {
        header = new Node(K(), V(), MAX_LEVEL - 1);   // 头节点要占满所有层，只当各层的起点
    }

    ~SkipList() {
        // level 0 串着所有节点，顺着它删一遍就行
        Node* p = header;
        while (p != nullptr) {
            Node* next = p->forward[0];
            delete p;
            p = next;
        }
    }

    // 节点都是 new 出来的，拷一份会 double free，干脆禁掉
    SkipList(const SkipList&) = delete;
    SkipList& operator=(const SkipList&) = delete;

    int size() const { return nodeCount; }
    bool empty() const { return nodeCount == 0; }
    int levels() const { return level + 1; }     // 当前一共几层

    // 查找。找到就把 value 带出来，返回 true
    bool search(const K& key, V& value) const {
        const Node* node = header;
        for (int i = level; i >= 0; --i) {
            // 在这一层一直往前，直到下一个节点的 key 不小于 key
            while (node->forward[i] != nullptr && node->forward[i]->key < key)
                node = node->forward[i];
        }
        node = node->forward[0];            // 落到 level 0，看下一个是不是它
        if (node != nullptr && node->key == key) {
            value = node->value;
            return true;
        }
        return false;
    }

    // 插入，已经有这个 key 就不插，返回 false
    bool insert(const K& key, const V& value) {
        Node* update[MAX_LEVEL];            // update[i] 记下第 i 层上要接在谁后面
        Node* node = header;
        for (int i = level; i >= 0; --i) {
            while (node->forward[i] != nullptr && node->forward[i]->key < key)
                node = node->forward[i];
            update[i] = node;
        }

        Node* next = node->forward[0];
        if (next != nullptr && next->key == key) return false;   // 不存重复的 key

        int newLevel = randomLevel();
        if (newLevel > level) {
            // 新节点比现在的层数还高，多出来的那几层直接从 header 接出去
            for (int i = level + 1; i <= newLevel; ++i) update[i] = header;
            level = newLevel;
        }

        Node* fresh = new Node(key, value, newLevel);
        for (int i = 0; i <= newLevel; ++i) {
            fresh->forward[i] = update[i]->forward[i];
            update[i]->forward[i] = fresh;
        }
        ++nodeCount;
        return true;
    }

    // 删除。删掉了返回 true，原来的 value 也一起带出来
    bool remove(const K& key, V& value) {
        Node* update[MAX_LEVEL];
        Node* node = header;
        for (int i = level; i >= 0; --i) {
            while (node->forward[i] != nullptr && node->forward[i]->key < key)
                node = node->forward[i];
            update[i] = node;
        }

        Node* target = node->forward[0];
        if (target == nullptr || target->key != key) return false;

        value = target->value;
        for (int i = 0; i <= level; ++i) {
            if (update[i]->forward[i] != target) break;   // 再往上就没有这个节点了
            update[i]->forward[i] = target->forward[i];
        }
        delete target;

        // 顶上那几层可能已经空了，把 level 降下来
        while (level > 0 && header->forward[level] == nullptr) --level;
        --nodeCount;
        return true;
    }

    // 按顺序打印所有元素（level 0 就是完整的链表）
    void printAll() const {
        cout << "按顺序遍历: ";
        for (const Node* p = header->forward[0]; p != nullptr; p = p->forward[0])
            cout << p->key << "=" << p->value << "  ";
        cout << '\n';
    }

    // 把每一层都打出来，看看索引是怎么搭的
    void dump() const {
        cout << "跳表结构（" << nodeCount << " 个元素，" << levels() << " 层）：\n";
        for (int i = level; i >= 0; --i) {
            cout << "  L" << i << ": ";
            for (const Node* p = header->forward[i]; p != nullptr; p = p->forward[i])
                cout << p->key << " ";
            cout << '\n';
        }
    }

private:
    struct Node {
        K key;
        V value;
        Node** forward;         // forward[i] 是这一层上的下一个节点
        int nodeLevel;

        Node(const K& k, const V& v, int lv) : key(k), value(v), nodeLevel(lv) {
            forward = new Node*[lv + 1]();   // 加括号是为了先把每个指针都置成 nullptr
        }
        ~Node() { delete[] forward; }

        Node(const Node&) = delete;
        Node& operator=(const Node&) = delete;
    };

    // 掷硬币决定新节点有几层：每往上多一层的概率是一半
    int randomLevel() {
        int lv = 0;
        while (lv < MAX_LEVEL - 1 && (rng() & 1)) ++lv;
        return lv;
    }

    Node* header = nullptr;
    int level = 0;              // 当前最高层，0 表示只有最底下那层
    int nodeCount = 0;
    mt19937 rng;
};

int main() {
    // 固定种子，跑出来结构一样方便对照；换成随机种子结果也一样对
    SkipList<int, string> list(20261012);

    cout << "=== 空表先试一下 ===\n";
    string v;
    cout << "size = " << list.size() << "，empty = " << (list.empty() ? "true" : "false") << '\n';
    cout << "查 5：" << (list.search(5, v) ? "找到" : "没找到") << '\n';
    cout << "删 5：" << (list.remove(5, v) ? "删掉了" : "没有这个节点") << '\n';
    cout << '\n';

    cout << "=== 插入 1~20 ===\n";
    for (int i = 1; i <= 20; ++i)
        list.insert(i, "v" + to_string(i));
    cout << "size = " << list.size() << '\n';
    list.printAll();
    list.dump();
    cout << '\n';

    cout << "=== 重复插入同一个 key ===\n";
    cout << "再插一次 7：" << (list.insert(7, "重复") ? "插进去了" : "已经存在，没插") << '\n';
    list.search(7, v);
    cout << "7 的值还是 " << v << "，size 还是 " << list.size() << '\n';
    cout << '\n';

    cout << "=== 查找 ===\n";
    for (int key : {1, 7, 20, 21, 0}) {
        if (list.search(key, v)) cout << "查 " << key << "：找到，值 = " << v << '\n';
        else                     cout << "查 " << key << "：没找到\n";
    }
    cout << '\n';

    cout << "=== 删除 ===\n";
    for (int key : {1, 20, 7, 7, 99}) {
        if (list.remove(key, v)) cout << "删 " << key << "：成功，原来的值 = " << v << '\n';
        else                     cout << "删 " << key << "：没有这个节点\n";
    }
    cout << "size = " << list.size() << '\n';
    list.printAll();
    list.dump();
    cout << '\n';

    cout << "=== 一个个删空，看层数会不会跟着降 ===\n";
    for (int i = 0; i <= 20; ++i) list.remove(i, v);
    cout << "size = " << list.size() << "，empty = " << (list.empty() ? "true" : "false")
         << "，层数 = " << list.levels() << '\n';
    list.dump();
    cout << '\n';

    // key 换成 string 照样能用，说明模板没白写
    cout << "=== 换个 key 类型（string）===\n";
    SkipList<string, int> words(7);
    words.insert("banana", 2);
    words.insert("apple", 1);
    words.insert("cherry", 3);
    words.insert("apple", 99);          // 重复的插不进去
    words.printAll();
    int n = 0;
    cout << "查 cherry：" << (words.search("cherry", n) ? to_string(n) : "没找到") << '\n';
    words.remove("apple", n);
    words.printAll();

    return 0;
}
