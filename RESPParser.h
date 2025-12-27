#pragma once
#include <string>
#include <vector>
#include <stdexcept>
#include <unistd.h> 
#include "RedSrvr.h"
using namespace std;

constexpr int READ_CACHE_MAX = 4096;  
constexpr int ITEM_LEN_MAX   = 1024 * 1024; 
const string NULL_BULK_STRING = "$-1\r\n";


// Exception types
struct Sys_Call_Fail : public runtime_error {
    Sys_Call_Fail(const string& msg) : runtime_error(msg) {}
};

struct Inc_Proto : public runtime_error {
    Inc_Proto(const string& msg) : runtime_error(msg) {}
};

/*
RESP parser for Redis-like protocol
Reads RESP arrays of bulk strings from a file descriptor
*/
class RESPParser {
private:
    int read_file_desc;           // FD to read from
    string read_cache; // buffer for partial reads

    // Helpers
    string read_fd(int n_bytes);
    bool cache_vitem(string& item);
    void upd_cache();

    bool varr_sz(const string& size_item);
    bool vbstr_sz(const string& size_item);
    bool v_crlf(const string& bstr);

public:
    RESPParser(int fd) : read_file_desc(fd), read_cache() {}

    string read_nxt_itm();
    vector<string> read_new_req();
};
