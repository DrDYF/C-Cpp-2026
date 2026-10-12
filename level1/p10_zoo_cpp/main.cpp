#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace std;

/*
 * 动物园
 *
 * Animal 是抽象基类，只规定“叫什么”和“怎么叫”。
 * 具体叫声交给各个子类去实现，Zoo 只管拿着 Animal 指针挨个调用 ——
 * 所以以后加新动物，Zoo 的代码一行都不用改。
 *
 * 动物用 unique_ptr 存着，不用自己 delete，也不会漏删。
 */
class Animal {
public:
    virtual ~Animal() = default;            // 基类析构得是虚的，不然删子类对象时析构不全
    virtual string name() const = 0;
    virtual string sound() const = 0;
};

class Dog : public Animal {
public:
    string name() const override { return "狗"; }
    string sound() const override { return "汪汪汪"; }
};

class Cat : public Animal {
public:
    string name() const override { return "猫"; }
    string sound() const override { return "喵~喵~"; }
};

// ---- 功能要求（二）：不动 Zoo，直接加新动物 ----

class Bird : public Animal {
public:
    string name() const override { return "鸟"; }
    string sound() const override { return "叽叽喳喳"; }
};

// 狼狗也是狗，只是叫得不一样
class WolfDog : public Dog {
public:
    string name() const override { return "狼狗"; }
    string sound() const override { return "嗷呜~~~~"; }
};

class Zoo {
public:
    // 收下一只动物。空指针直接拒掉，免得点名时崩掉。
    bool add(unique_ptr<Animal> animal) {
        if (!animal) return false;
        animals_.push_back(move(animal));
        return true;
    }

    // 让园里所有动物依次吼一次
    void rollCall() const {
        if (animals_.empty()) {
            cout << "动物园里还没有动物，跳过。\n";
            return;
        }
        cout << "动物园点名，共 " << animals_.size() << " 只：\n";
        for (const auto& animal : animals_)
            cout << "  " << animal->name() << ": " << animal->sound() << '\n';
    }

    int size() const { return (int)animals_.size(); }

private:
    vector<unique_ptr<Animal>> animals_;
};

int main() {
    Zoo zoo;

    // 空园也点一次名，不然会打印出一句没头没尾的“共 0 只”
    zoo.rollCall();

    zoo.add(make_unique<Dog>());
    zoo.add(make_unique<Cat>());
    zoo.add(make_unique<Bird>());       // 新加的鸟
    zoo.add(make_unique<WolfDog>());    // 新加的狼狗

    // 塞个空指针进来也不会出事
    if (!zoo.add(nullptr))
        cout << "空的动物指针，没收。\n";

    zoo.rollCall();

    // 上面加的鸟和狼狗，Zoo 类一个字都没动
    cout << "园里一共 " << zoo.size() << " 只动物。\n";
    return 0;
}
