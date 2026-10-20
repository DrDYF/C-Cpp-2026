#include <iostream>
#include <fstream>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>
#include <limits>

using namespace std;

// 一种货物
struct Item {
    string model;     // 型号
    int quantity;     // 数量
};

bool isValidModel(const string& model) {
    if (model.empty() || model.size() > 64) return false;
    for (unsigned char c : model)
        if (isspace(c)) return false;
    return true;
}

bool isValidItem(const Item& item) {
    return isValidModel(item.model) && item.quantity >= 0;
}

bool hasModel(const vector<Item>& stock, const string& model) {
    for (const Item& item : stock)
        if (item.model == model) return true;
    return false;
}

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

bool parseStockLine(const string& line, Item& item) {
    istringstream input(line);
    string extra;
    if (!(input >> item.model >> item.quantity) || (input >> extra)) return false;
    return isValidItem(item);
}

// 文件里一行存一条记录：型号 数量。发现任何坏行时不加载部分库存。
bool loadStock(const string& path, vector<Item>& stock) {
    stock.clear();
    ifstream fin(path);
    if (!fin) {
        cout << "还没用过这个程序吧？先按空仓库来。\n";
        return true;
    }

    vector<Item> loaded;
    string line;
    int lineNumber = 0;
    while (getline(fin, line)) {
        ++lineNumber;
        if (!line.empty() && line.back() == '\r') line.pop_back();

        Item item;
        if (!parseStockLine(line, item)) {
            cout << "库存文件第 " << lineNumber << " 行格式无效，未加载库存。\n";
            return false;
        }
        if (hasModel(loaded, item.model)) {
            cout << "库存文件第 " << lineNumber << " 行型号重复，未加载库存。\n";
            return false;
        }
        loaded.push_back(item);
    }
    if (fin.bad()) {
        cout << "库存文件读取失败，未加载库存。\n";
        return false;
    }

    stock.swap(loaded);
    return true;
}

bool saveStock(const string& path, const vector<Item>& stock) {
    for (size_t i = 0; i < stock.size(); ++i) {
        if (!isValidItem(stock[i])) return false;
        for (size_t j = 0; j < i; ++j)
            if (stock[j].model == stock[i].model) return false;
    }

    ofstream fout(path);
    if (!fout) return false;
    for (const Item& item : stock)
        fout << item.model << ' ' << item.quantity << '\n';
    fout.flush();
    if (!fout) return false;
    fout.close();
    return !fout.fail();
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
bool readModelAndCount(string& model, int& quantity) {
    cout << "输入型号和数量（例如 A100 20）: ";
    if (!(cin >> model >> quantity) || !isValidModel(model) || quantity <= 0) {
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
    int quantity;
    if (!readModelAndCount(model, quantity)) return;

    int idx = findItem(stock, model);
    if (idx == -1) {
        stock.push_back({model, quantity});
        cout << "新增货物 " << model << "，数量 " << quantity << '\n';
    } else {
        if (quantity > numeric_limits<int>::max() - stock[idx].quantity) {
            cout << "数量超出范围，入库失败。\n";
            return;
        }
        stock[idx].quantity += quantity;
        cout << model << " 入库 " << quantity << "，现在有 " << stock[idx].quantity << '\n';
    }
}

// 出库：库存不够就整单拒绝，免得出现负数
void stockOut(vector<Item>& stock) {
    string model;
    int quantity;
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

    if (!loadStock(path, stock)) {
        cout << "请修复库存文件后重新启动，原文件未修改。\n";
        return 1;
    }
    cout << "库存数据来自: " << path << '\n';

    while (true) {
        printMenu();

        int choice;
        if (!(cin >> choice)) break;    // 输入了非数字就当作退出

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
                if (saveStock(path, stock))
                    cout << "库存已保存到 " << path << "，再见。\n";
                else
                    cout << "保存失败，库存文件未能完整写入。\n";
                return 0;
            default:
                cout << "没有这个选项，重新选一下。\n";
        }
    }

    if (!saveStock(path, stock))
        cout << "保存失败，库存文件未能完整写入。\n";
    return 0;
}
