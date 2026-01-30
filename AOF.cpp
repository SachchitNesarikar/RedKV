#include "AOF.h"
#include "cmd.h"
#include "RedKV.h"

#include <fstream>
#include <atomic>
#include <sstream>
#include "utils.h"

static std::atomic<bool> loading{false};
static std::ofstream aof;

AOF& AOF::instance() {
    static AOF inst;
    return inst;
}

void AOF::open() {
    aof.open("appendonly.aof", std::ios::app);
}

bool AOF::is_loading() const {
    return loading;
}

void AOF::append(const std::vector<std::string>& cmd) {
    if (loading) return;
    if (!aof.is_open()) return;

    for (size_t i = 0; i < cmd.size(); i++) {
        aof << cmd[i];
        if (i + 1 < cmd.size()) aof << ' ';
    }
    aof << '\n';
    aof.flush();
}

void AOF::replay() {
    std::ifstream in("appendonly.aof");
    if (!in.is_open()) return;

    loading = true;

    std::string line;
    while (std::getline(in, line)) {
        std::istringstream iss(line);
        std::vector<std::string> cmd;
        std::string tok;

        while (iss >> tok)
            cmd.push_back(tok);

        if (cmd.empty()) continue;

        cmd[0] = to_lower(cmd[0]);
        auto fn = get_redis_func(cmd[0]);
        if (fn) fn(cmd);
    }

    loading = false;
}
