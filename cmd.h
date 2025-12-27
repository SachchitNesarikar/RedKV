#pragma once

#include <vector>
#include <string>
#include <memory>
#include <functional>
#include "RedSrvr.h"
using namespace std;

using cmd_retn = unique_ptr<redis::RedisObject>;
using red_ftype = function<cmd_retn(const vector<string>&)>;

// dispatch
red_ftype get_redis_func(const string&);

// commands
cmd_retn redis_ping(const vector<string>&);
cmd_retn redis_echo(const vector<string>&);
cmd_retn redis_set (const vector<string>&);
cmd_retn redis_get (const vector<string>&);
cmd_retn redis_config(const vector<string>&);
