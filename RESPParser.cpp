#include "RESPParser.h"
#include <sys/socket.h>
#include <string>
#include <vector>
#include <iostream>
#include "RedSrvr.h"
using namespace std;

string RESPParser::read_fd(int n) {
    vector<char> buf(n);
    ssize_t r = recv(read_file_desc, buf.data(), n, 0);

    if (r == 0) {
        throw runtime_error("client closed");
    }
    if (r < 0) {
        throw Sys_Call_Fail("recv failed");
    }

    return string(buf.data(), r);
}

bool RESPParser::cache_vitem(string& item) {
    for (size_t i = 0; i < read_cache.size(); i++) {
        item += read_cache[i];
        if (item.size() >= 2 &&
            item[item.size() - 2] == '\r' &&
            item[item.size() - 1] == '\n') {
            read_cache = read_cache.substr(item.size());
            return true;
        }
    }
    return false;
}

void RESPParser::upd_cache() { read_cache += read_fd(READ_CACHE_MAX); }

string RESPParser::read_nxt_itm() {
    string item;
    while (!cache_vitem(item)) {
        if (item.size() > ITEM_LEN_MAX) throw Inc_Proto("item length too big!");
        upd_cache();
    }
    return item;
}

bool RESPParser::varr_sz(const string& sz_item) {
    size_t sz = sz_item.size();
    if (sz < 4 || sz_item[0] != '*' || sz_item[sz - 2] != '\r' || sz_item[sz - 1] != '\n') return false;
    for (size_t i = 1; i < sz - 2; ++i) if (!isdigit(sz_item[i])) return false;
    return true;
}

bool RESPParser::vbstr_sz(const string& sz_item) {
    size_t sz = sz_item.size();
    if (sz < 4 || sz_item[0] != '$' || sz_item[sz - 2] != '\r' || sz_item[sz - 1] != '\n') return false;
    for (size_t i = 1; i < sz - 2; ++i) if (!isdigit(sz_item[i])) return false;
    return true;
}

bool RESPParser::v_crlf(const string& bstr) {
    size_t sz = bstr.size();
    return sz >= 2 && bstr[sz - 2] == '\r' && bstr[sz - 1] == '\n';
}

vector<string> RESPParser::read_new_req() {
    string arr_size_item = read_nxt_itm();
    if (!varr_sz(arr_size_item)) throw Inc_Proto("Bad array size");

    int cnt = stoi(arr_size_item.substr(1, arr_size_item.size() - 3));
    vector<string> req(cnt);

    for (int i = 0; i < cnt; ++i) {
        string bstr_size_item = read_nxt_itm();
        if (!vbstr_sz(bstr_size_item)) throw Inc_Proto("Bad bulk string size");

        int bstr_sz = stoi(bstr_size_item.substr(1, bstr_size_item.size() - 3));
        if (bstr_sz == -1) {
            req[i] = NULL_BULK_STRING;
            continue;
        }
        if (bstr_sz < -1) throw Inc_Proto("Bulk string size < -1");

        string bstr_item = read_nxt_itm();
        if (!v_crlf(bstr_item)) throw Inc_Proto("Bulk string not terminated by CRLF");

        string bstr = bstr_item.substr(0, bstr_item.size() - 2);
        if ((int)bstr.size() != bstr_sz) throw Inc_Proto("Bulk string size mismatch");

        req[i] = bstr;
    }

    return req;
}
