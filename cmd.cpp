#include "cmd.h"
#include "RedKV.h"
#include "RedSrvr.h"
using namespace std;

#define ll long long

// Map Redis command -> handler
static unordered_map<string, red_ftype> cmd_map = {
    {"ping",   redis_ping},
    {"echo",   redis_echo},
    {"set",    redis_set},
    {"get",    redis_get},
    {"config", redis_config},
    {"del", redis_del},
    {"exists", redis_exists},
    {"ttl", redis_ttl},
};

// Lookup command
red_ftype get_redis_func(const string& cmd_name) {
    auto it = cmd_map.find(cmd_name);
    if (it != cmd_map.end()) return it->second;
    return red_ftype{}; // empty function
}

cmd_retn redis_ping(const vector<string>& req) {
    if (req.empty() || req[0] != "ping") throw RedisServerError("Bad input");
    if (req.size() > 2) return make_unique<redis::Error>("ERR wrong number of arguments for 'ping' command");
    if (req.size() == 1) return make_unique<redis::SimpleString>("PONG");
    return make_unique<redis::SimpleString>(req[1]);
}

cmd_retn redis_echo(const vector<string>& req) {
    if (req.empty() || req[0] != "echo") throw RedisServerError("Bad input");
    if (req.size() != 2) return make_unique<redis::Error>("ERR wrong number of arguments for 'echo' command");
    return make_unique<redis::BulkString>(req[1]);
}

cmd_retn redis_set(const vector<string>& req) {
    if (req.empty() || req[0] != "set") throw RedisServerError("Bad input");
    if (req.size() < 3) return make_unique<redis::Error>("ERR syntax error");

    time_t expiry = numeric_limits<time_t>::max();
    ll i = 3;
    while (i < req.size()) {
        if (req[i] == "EX") { if (++i >= req.size()) return make_unique<redis::Error>("ERR syntax error"); expiry = time(nullptr) + stol(req[i]); }
        else if (req[i] == "PX") { if (++i >= req.size()) return make_unique<redis::Error>("ERR syntax error"); expiry = time(nullptr) + stol(req[i]) / 1000; }
        else if (req[i] == "EXAT") { if (++i >= req.size()) return make_unique<redis::Error>("ERR syntax error"); expiry = stol(req[i]); }
        else if (req[i] == "PXAT") { if (++i >= req.size()) return make_unique<redis::Error>("ERR syntax error"); expiry = stol(req[i]) / 1000; }
        else return make_unique<redis::Error>("ERR syntax error");
        i++;
    }

    RedKV::getInstance().set(req[1], req[2], expiry);
    return make_unique<redis::SimpleString>("OK");
}

cmd_retn redis_get(const vector<string>& req) {
    if (req.empty() || req[0] != "get") throw RedisServerError("Bad input");
    if (req.size() != 2) return make_unique<redis::Error>("ERR wrong number of arguments for 'get' command");

    string out;
    bool found = RedKV::getInstance().get(req[1], out);
    if (!found) return make_unique<redis::NullString>();
    return make_unique<redis::BulkString>(out);
}

cmd_retn redis_config(const vector<string>& req) {
    auto arr = make_unique<redis::Array>();
    arr->add_element(make_unique<redis::BulkString>("900"));
    arr->add_element(make_unique<redis::BulkString>("1"));
    return arr;
}

cmd_retn redis_del(const vector<string>& req) { 
    if (req.size() < 2) return make_unique<redis::Error>("ERR wrong number of arguments for 'del' command"); 
    
    ll cnt = 0; 
    for (ll i = 1; i < req.size(); i++) if (RedKV::getInstance().del(req[i])) ++cnt; 
    return make_unique<redis::Integer>(cnt); 
}

cmd_retn redis_exists(const vector<string>& req) {
    if (req.size() < 2) return make_unique<redis::Error>("ERR wrong number of arguments for 'exists' command");

    ll cnt = 0;
    for (size_t i = 1; i < req.size(); i++) if (RedKV::getInstance().exists(req[i])) cnt++;
    return make_unique<redis::Integer>(cnt);
}

cmd_retn redis_ttl(const vector<string>& req) {
    if (req.size() != 2) return make_unique<redis::Error>("ERR wrong number of arguments for 'ttl' command");

    ll res = RedKV::getInstance().ttl(req[1]);
    return make_unique<redis::Integer>(res);
}
