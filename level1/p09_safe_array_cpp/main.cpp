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
 *
 * 内存是类自己 new 出来的，所以拷贝、移动、析构都得自己写：
 * 拷贝要重新分一块内存（不然两个对象析构时会 double free），
 * 移动直接把指针接过来就行。
 */
template <typename T>
class SafeArray {
public:
    explicit SafeArray(int size) : size_(0), data_(nullptr) {
        if (size < 0)
            throw invalid_argument("数组长度不能为负数: " + to_string(size));
        size_ = size;
        data_ = new T[size]();          // 加个括号是为了把元素都初始化一下
    }

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

    // 移动构造：把指针直接接过来，原来的对象交出去之后置空
    SafeArray(SafeArray&& other) noexcept
        : size_(other.size_), data_(other.data_) {
        other.size_ = 0;
        other.data_ = nullptr;
    }

    SafeArray& operator=(SafeArray&& other) noexcept {
        if (this != &other) {
            delete[] data_;
            size_ = other.size_;
            data_ = other.data_;
            other.size_ = 0;
            other.data_ = nullptr;
        }
        return *this;
    }

    ~SafeArray() { delete[] data_; }

    int size() const { return size_; }
    bool empty() const { return size_ == 0; }

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
        if (index < 0 || index >= size_) {
            // 空数组没有合法下标，单独说清楚
            if (size_ == 0)
                throw out_of_range("数组长度为 0，任何下标都越界（index = "
                                   + to_string(index) + "）");
            throw out_of_range("下标越界: " + to_string(index)
                               + "（合法范围 0 ~ " + to_string(size_ - 1) + "）");
        }
    }

    void swap(SafeArray& other) noexcept {
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

// 长度给成负数是用错了，直接报错，别去申请一块天文数字的内存
void tryCreate(int size) {
    try {
        SafeArray<int> bad(size);
        cout << "  创建长度 " << size << " 的数组成功\n";
    } catch (const invalid_argument& e) {
        cout << "  创建长度 " << size << " 的数组被拦住了 -> " << e.what() << '\n';
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

    // 空数组和负长度这两种情况也试一下
    cout << "边界情况：\n";
    SafeArray<int> none(0);
    cout << "  空数组 size() = " << none.size()
         << "，empty() = " << (none.empty() ? "true" : "false") << '\n';
    tryRead(none, 0);
    tryCreate(-3);

    // 拷贝是深拷贝：改副本不会动到原件
    SafeArray<int> copy(nums);
    copy[0] = 999;
    cout << "深拷贝检查：原件 nums[0] = " << nums[0]
         << "，副本 copy[0] = " << copy[0] << '\n';

    // 移动之后数据搬到新对象身上，原对象成了空壳
    SafeArray<int> moved(std::move(copy));
    cout << "移动检查：moved.size() = " << moved.size()
         << "，被搬走的 copy.size() = " << copy.size() << '\n';

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
