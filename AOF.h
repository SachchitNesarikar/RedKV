#pragma once
#include <fstream>
#include <mutex>
#include <vector>
#include <string>
using namespace std;

class AOF {
private:
    std::ofstream ofs;
    std::mutex mtx;
    bool loading = false;

    AOF();
    ~AOF();

public:
    static AOF& instance();

    void append(const std::vector<std::string>& cmd);
    void load();

    bool is_loading() const { return loading; }
};
