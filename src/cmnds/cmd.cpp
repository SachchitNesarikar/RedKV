#include "cmd.h"
#include "../protocol/RESPParser.h"
#include <algorithm>
#include <cctype>

std::string cmd::to_lower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return result;
}

std::string cmd::execute(const std::vector<std::string>& cmd, RedKV& kv) {
    if (cmd.empty()) {
        return RESPSerializer::error("ERR empty command");
    }
    
    std::string cmd_name = to_lower(cmd[0]);
    
    if (cmd_name == "ping") return cmd_ping(cmd);
    else if (cmd_name == "echo") return cmd_echo(cmd);
    else if (cmd_name == "get") return cmd_get(cmd, kv);
    else if (cmd_name == "set") return cmd_set(cmd, kv);
    else if (cmd_name == "del") return cmd_del(cmd, kv);
    else if (cmd_name == "exists") return cmd_exists(cmd, kv);
    else if (cmd_name == "ttl") return cmd_ttl(cmd, kv);
    else if (cmd_name == "dbsize") return cmd_dbsize(cmd, kv);
    else if (cmd_name == "command") return RESPSerializer::array({});
    else return RESPSerializer::error("ERR unknown command '" + cmd_name + "'");
}

std::string cmd::cmd_ping(const std::vector<std::string>& args) {
    if (args.size() > 2) return RESPSerializer::error("ERR wrong number of arguments for 'ping' command");
    
    if (args.size() == 2) return RESPSerializer::simple_string(args[1]);
    
    return RESPSerializer::simple_string("PONG");
}

std::string cmd::cmd_echo(const std::vector<std::string>& args) {
    if (args.size() != 2) return RESPSerializer::error("ERR wrong number of arguments for 'echo' command");
    
    return RESPSerializer::bulk_string(args[1]);
}

std::string cmd::cmd_get(const std::vector<std::string>& args, RedKV& kv) {
    if (args.size() != 2) return RESPSerializer::error("ERR wrong number of arguments for 'get' command");
    
    std::string value;
    if (kv.get(args[1], value)) return RESPSerializer::bulk_string(value);
    
    return RESPSerializer::null_bulk_string();
}

std::string cmd::cmd_set(const std::vector<std::string>& args, RedKV& kv) {
    if (args.size() < 3) {
        return RESPSerializer::error("ERR wrong number of arguments for 'set' command");
    }
    
    const std::string& key = args[1];
    const std::string& value = args[2];
    
    time_t expiry = 0;
    for (size_t i = 3; i < args.size(); i++) {
        std::string opt = to_lower(args[i]);
        
        if (opt == "ex") {
            if (i + 1 >= args.size()) {
                return RESPSerializer::error("ERR syntax error");
            }
            try {
                expiry = std::stoll(args[i + 1]);
                i++;
            } catch (...) {
                return RESPSerializer::error("ERR value is not an integer or out of range");
            }
        } else if (opt == "px") {
            if (i + 1 >= args.size()) {
                return RESPSerializer::error("ERR syntax error");
            }
            try {
                expiry = std::stoll(args[i + 1]) / 1000;
                i++;
            } catch (...) {
                return RESPSerializer::error("ERR value is not an integer or out of range");
            }
        } else {
            return RESPSerializer::error("ERR syntax error");
        }
    }
    
    if (expiry > 0) kv.set(key, value, expiry);
    else kv.set(key, value);
    
    return RESPSerializer::simple_string("OK");
}

std::string cmd::cmd_del(const std::vector<std::string>& args, RedKV& kv) {
    if (args.size() < 2) {
        return RESPSerializer::error("ERR wrong number of arguments for 'del' command");
    }
    
    long long cnt = 0;
    for (size_t i = 1; i < args.size(); i++) if (kv.del(args[i])) cnt++;
    return RESPSerializer::integer(cnt);
}

std::string cmd::cmd_exists(const std::vector<std::string>& args, RedKV& kv) {
    if (args.size() < 2) return RESPSerializer::error("ERR wrong number of arguments for 'exists' command");
    
    long long cnt = 0;
    for (size_t i = 1; i < args.size(); i++) if (kv.exists(args[i])) cnt++;
    return RESPSerializer::integer(cnt);
}

std::string cmd::cmd_ttl(const std::vector<std::string>& args, RedKV& kv) {
    if (args.size() != 2) return RESPSerializer::error("ERR wrong number of arguments for 'ttl' command");    
    long long ttl = kv.ttl(args[1]);
    return RESPSerializer::integer(ttl);
}

std::string cmd::cmd_dbsize(const std::vector<std::string>& args, RedKV& kv) {
    if (args.size() != 1) return RESPSerializer::error("ERR wrong number of arguments for 'dbsize' command");
    return RESPSerializer::integer(kv.dbsize());
}