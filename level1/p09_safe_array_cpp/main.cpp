#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

using namespace std;

/*
 * 安全数组
 *
 * 裸数组越界不会报错，只会悄悄踩到别的内存，很难查。
 * 这里用类把它包起来，下标不对就抛 out_of_range，程序能自己发现错误。
 *
 * 用模板写的，所以 int / double / string …… 什么类型都能用。
 */
template <typename T>
class SafeArray {
public:
    explicit SafeArray(int size) : size_(size), data_(new T[size]()) {}

    // 拷贝构造：不然两个对象会指向同一块内存，析构时 double free
    SafeArray(const SafeArray& other) : size_(other.size_), data_(new T[other.size_]) {
        for (int i = 0; i < size_; ++i)
            data_[i] = other.data_[i];
    }

    SafeArray& operator=(const SafeArray& other) {
        if (this != &other) {
            SafeArray copy(other);      // 先拷一份再换，中途出异常也不会把自己弄坏
            swap(copy);
        }
        return *this;
    }

    ~SafeArray() { delete[] data_; }

    int size() const { return size_; }

    // 功能要求（二）：重载下标运算符，顺便做越界检查
    T& operator[](int index) {
        checkIndex(index);
        return data_[index];
    }

    const T& operator[](int index) const {
        checkIndex(index);
        return data_[index];
    }

private:
    void checkIndex(int index) const {
        if (index < 0 || index >= size_)
            throw out_of_range("下标越界: " + to_string(index)
                               + "（合法范围 0 ~ " + to_string(size_ - 1) + "）");
    }

    void swap(SafeArray& other) {
        std::swap(size_, other.size_);
        std::swap(data_, other.data_);
    }

    int size_;
    T* data_;
};

// 下面两个小函数只是拿来演示：越界时会被拦住，而不是踩坏内存
void tryRead(const SafeArray<int>& arr, int index) {
    try {
        int value = arr[index];     // 越界的话这一行就抛出去了
        cout << "  读 arr[" << index << "] = " << value << '\n';
    } catch (const out_of_range& e) {
        cout << "  读 arr[" << index << "] 被拦住了 -> " << e.what() << '\n';
    }
}

void tryWrite(SafeArray<int>& arr, int index, int value) {
    try {
        arr[index] = value;
        cout << "  写 arr[" << index << "] 成功\n";
    } catch (const out_of_range& e) {
        cout << "  写 arr[" << index << "] 被拦住了 -> " << e.what() << '\n';
    }
}

int main() {
    SafeArray<int> nums(5);
    for (int i = 0; i < nums.size(); ++i)
        nums[i] = (i + 1) * 10;

    cout << "nums = ";
    for (int i = 0; i < nums.size(); ++i)
        cout << nums[i] << (i + 1 < nums.size() ? ", " : "\n");

    // 功能要求（一）：越界要报错。读写都试一下
    cout << "试着越界访问：\n";
    tryRead(nums, 10);
    tryRead(nums, -1);
    tryWrite(nums, 5, 0);

    // 功能要求（三）：换个类型照样能用
    SafeArray<string> words(3);
    words[0] = "hello";
    words[1] = "safe";
    words[2] = "array";
    cout << "words = ";
    for (int i = 0; i < words.size(); ++i)
        cout << words[i] << (i + 1 < words.size() ? " " : "\n");

    return 0;
}
