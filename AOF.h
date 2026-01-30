#pragma once
#include <string>
#include <vector>

class AOF {
public:
    static AOF& instance();

    void open();
    void append(const std::vector<std::string>& cmd);
    void replay();
    bool is_loading() const;

private:
    AOF() = default;
};
