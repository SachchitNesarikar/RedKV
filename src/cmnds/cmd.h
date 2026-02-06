#pragma once

#include <string>
#include <vector>
#include "../store/RedKV.h"

class cmd {
public:
    static std::string execute(const std::vector<std::string>& cmd, RedKV& kv);

private:
    static std::string cmd_ping(const std::vector<std::string>& args);
    static std::string cmd_echo(const std::vector<std::string>& args);
    static std::string cmd_get(const std::vector<std::string>& args, RedKV& kv);
    static std::string cmd_set(const std::vector<std::string>& args, RedKV& kv);
    static std::string cmd_del(const std::vector<std::string>& args, RedKV& kv);
    static std::string cmd_exists(const std::vector<std::string>& args, RedKV& kv);
    static std::string cmd_ttl(const std::vector<std::string>& args, RedKV& kv);
    static std::string cmd_dbsize(const std::vector<std::string>& args, RedKV& kv);
    
    static std::string to_lower(const std::string& s);
};