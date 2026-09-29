#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace std;

/*
 * 动物园
 *
 * Animal 是抽象基类，只规定“叫什么”和“怎么叫”。
 * 具体叫声交给各个子类去实现，Zoo 只管拿着 Animal 指针挨个调用 ——
 * 所以以后加新动物，Zoo 的代码一行都不用改。
 */
class Animal {
public:
    virtual ~Animal() = default;
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
    void add(unique_ptr<Animal> animal) {
        animals_.push_back(move(animal));
    }

    // 让园里所有动物依次吼一次
    void rollCall() const {
        cout << "动物园点名，共 " << animals_.size() << " 只：\n";
        for (const auto& animal : animals_)
            cout << "  " << animal->name() << ": " << animal->sound() << '\n';
    }

private:
    vector<unique_ptr<Animal>> animals_;
};

int main() {
    Zoo zoo;
    zoo.add(make_unique<Dog>());
    zoo.add(make_unique<Cat>());
    zoo.add(make_unique<Bird>());       // 新加的鸟
    zoo.add(make_unique<WolfDog>());    // 新加的狼狗

    zoo.rollCall();
    return 0;
}
