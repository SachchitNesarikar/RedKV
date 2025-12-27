#include "utils.h"
#include "RedSrvr.h"

void die(const char* msg) { perror(msg); exit(EXIT_FAILURE); }
void msg(const char* msg) { cerr << msg << endl; }
void print(const char* msg) { cout << msg << endl; }

string to_lower(const string& str) {
    string out = str;
    transform(out.begin(), out.end(), out.begin(), ::tolower);
    return out;
}

bool have_crlf(const char* buff, int sz) {
    if (sz < 2) return false;
    return buff[sz - 2] == '\r' && buff[sz - 1] == '\n';
}

// Recv exactly n bytes
int recv_exct(int f, vector<char>& buff, size_t n) {
    buff.resize(n);
    size_t off = 0;

    while (n > 0) {
        ssize_t r = recv(f, buff.data() + off, n, 0);
        if (r <= 0) return -1;
        assert(static_cast<size_t>(r) <= n);
        n -= r;
        off += r;
    }
    return 0;
}

// Write exactly n bytes
int wrt_exct(int f, const vector<char>& buff, size_t n) {
    size_t off = 0;
    while (n > 0) {
        ssize_t r = send(f, buff.data() + off, n, 0);
        if (r <= 0) return -1;
        assert(static_cast<size_t>(r) <= n);
        n -= r;
        off += r;
    }
    return 0;
}

// Receive packet with 4-byte length prefix
int recv1(int f, vector<char>& buff) {
    vector<char> hdr(4);
    if (recv_exct(f, hdr, 4) == -1) return -1;

    uint32_t sz = 0;
    memcpy(&sz, hdr.data(), 4);
    if (sz > maxx) {
        msg("packet too long");
        return -1;
    }

    buff.resize(4 + sz + 1);
    memcpy(buff.data(), &sz, 4);

    if (recv_exct(f, buff, 4 + sz) == -1) return -1;

    buff[4 + sz] = '\0';
    stringstream ss;
    ss << "received packet of length: " << sz;
    print(ss.str().c_str());
    return 0;
}

// Send packet with 4-byte length prefix
int send1(int f, const vector<char>& buff) {
    uint32_t sz = buff.size();
    if (sz > maxx) {
        msg("packet too long");
        return -1;
    }

    vector<char> wbuf(4 + sz);
    memcpy(wbuf.data(), &sz, 4);
    memcpy(wbuf.data() + 4, buff.data(), sz);

    if (wrt_exct(f, wbuf, wbuf.size()) == -1) return -1;

    stringstream ss;
    ss << "sent packet of length: " << sz;
    print(ss.str().c_str());
    return 0;
}
