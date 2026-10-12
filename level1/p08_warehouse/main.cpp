#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <limits>
#include <stdexcept>

using namespace std;

// 一种货物
struct Item {
    string model;         // 型号
    long long quantity;   // 数量
};

// 库存文件放在源码旁边，这样在 CLion 的 build 目录下运行也能找得到
string dataFilePath() {
    string here = __FILE__;
    size_t slash = here.find_last_of("\\/");
    if (slash != string::npos) {
        string dir = here.substr(0, slash);
        if (ifstream(dir + "/main.cpp")) return dir + "/stock.txt";
    }
    return "stock.txt";
}

// 文件里一行存一条记录：型号 数量
// 读的时候顺手把格式不对的行报出来，免得数据悄悄丢了还不知道
bool loadStock(const string& path, vector<Item>& stock) {
    ifstream fin(path);
    if (!fin) {
        cout << "还没用过这个程序吧？先按空仓库来。\n";
        return true;                      // 第一次跑没有文件，不算出错
    }

    string line;
    int lineNo = 0, badLines = 0;
    while (getline(fin, line)) {
        ++lineNo;
        if (line.empty()) continue;

        Item item;
        try {
            size_t pos = line.find(' ');
            item.model = line.substr(0, pos);
            item.quantity = stoll(line.substr(pos + 1));   // 数字太大这里会抛异常
        } catch (const exception&) {
            ++badLines;
            cout << "第 " << lineNo << " 行读不出来，已跳过：" << line << '\n';
            continue;
        }
        if (item.model.empty() || item.quantity < 0) {
            ++badLines;
            cout << "第 " << lineNo << " 行数据不合法，已跳过：" << line << '\n';
            continue;
        }
        stock.push_back(item);
    }
    if (badLines > 0)
        cout << "提示：库存文件里有 " << badLines << " 行没读进来。\n";
    return true;
}

// 保存库存。写成功返回 true，失败返回 false，
// 调用方看着返回值决定要不要提示“已保存”
bool saveStock(const string& path, const vector<Item>& stock) {
    ofstream fout(path, ios::trunc);
    if (!fout) return false;              // 打不开：没权限、路径不对之类

    for (const Item& item : stock)
        fout << item.model << ' ' << item.quantity << '\n';

    fout.flush();
    if (!fout) return false;              // 写到一半失败了（比如磁盘满）
    fout.close();
    return !fout.fail();                  // 关闭时也会把最后的错误暴露出来
}

void showStock(const vector<Item>& stock) {
    cout << "\n------------ 当前库存 ------------\n";
    if (stock.empty()) {
        cout << "（仓库是空的）\n";
    } else {
        cout << "型号\t\t数量\n";
        for (const Item& item : stock)
            cout << item.model << "\t\t" << item.quantity << '\n';
    }
    cout << "----------------------------------\n";
}

// 按型号找货物，返回下标；找不到返回 -1
int findItem(const vector<Item>& stock, const string& model) {
    for (int i = 0; i < (int)stock.size(); ++i)
        if (stock[i].model == model) return i;
    return -1;
}

// 读一行“型号 数量”，格式不对就返回 false
bool readModelAndCount(string& model, long long& quantity) {
    cout << "输入型号和数量（例如 A100 20）: ";
    if (!(cin >> model >> quantity) || quantity <= 0) {
        cout << "输入不合法。\n";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        return false;
    }
    return true;
}

// 入库：已有型号就累加，没有就新建一条
void stockIn(vector<Item>& stock) {
    string model;
    long long quantity;
    if (!readModelAndCount(model, quantity)) return;

    int idx = findItem(stock, model);
    if (idx == -1) {
        stock.push_back({model, quantity});
        cout << "新增货物 " << model << "，数量 " << quantity << '\n';
        return;
    }

    // 先看看加上去会不会溢出，不然数量会翻成负数
    const long long maxQuantity = numeric_limits<long long>::max();
    if (stock[idx].quantity > maxQuantity - quantity) {
        cout << "入库失败：" << model << " 现有 " << stock[idx].quantity
             << "，再加 " << quantity << " 会超出数量上限。\n";
        return;
    }

    stock[idx].quantity += quantity;
    cout << model << " 入库 " << quantity << "，现在有 " << stock[idx].quantity << '\n';
}

// 出库：库存不够就整单拒绝，免得出现负数
void stockOut(vector<Item>& stock) {
    string model;
    long long quantity;
    if (!readModelAndCount(model, quantity)) return;

    int idx = findItem(stock, model);
    if (idx == -1) {
        cout << "仓库里没有 " << model << '\n';
    } else if (stock[idx].quantity < quantity) {
        cout << "库存不够，" << model << " 只剩 " << stock[idx].quantity << '\n';
    } else {
        stock[idx].quantity -= quantity;
        cout << model << " 出库 " << quantity << "，还剩 " << stock[idx].quantity << '\n';
    }
}

void printMenu() {
    cout << "\n===== 简单进销存 =====\n"
         << "  1. 显示存货列表\n"
         << "  2. 入库\n"
         << "  3. 出库\n"
         << "  0. 退出程序\n"
         << "请选择: ";
}

int main() {
    string path = dataFilePath();
    vector<Item> stock;

    loadStock(path, stock);
    cout << "库存数据来自: " << path << '\n';

    bool quit = false;
    while (!quit) {
        printMenu();

        int choice;
        if (!(cin >> choice)) {
            if (cin.eof()) break;         // 输入没了（比如管道读完），直接收工
            cin.clear();                  // 输成字母了，清掉重问一遍，别直接退出
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "请输入数字选项。\n";
            continue;
        }

        switch (choice) {
            case 1:
                showStock(stock);
                break;
            case 2:
                stockIn(stock);
                break;
            case 3:
                stockOut(stock);
                break;
            case 0:
                quit = true;
                break;
            default:
                cout << "没有这个选项，重新选一下。\n";
        }
    }

    // 保存成功才敢说“已保存”，失败了如实说一声
    if (saveStock(path, stock)) {
        cout << "库存已保存到 " << path << "，再见。\n";
        return 0;
    }
    cout << "保存失败！库存没写进 " << path
         << "，看看文件是不是被占用了，或者磁盘还有没有空间。\n";
    return 1;
}
