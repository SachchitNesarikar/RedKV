#pragma once

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <cerrno>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <stdexcept>
#include <algorithm>
using namespace std;

constexpr int PORT = 2000;
constexpr int maxx = 4096;

// Exception types
struct IncProto : public runtime_error { IncProto(const string& msg) : runtime_error(msg) {} };
struct SysCallFail : public runtime_error { SysCallFail(const string& msg) : runtime_error(msg) {} };
struct SrvrErr : public runtime_error { SrvrErr(const string& msg) : runtime_error(msg) {} };

// Utility functions
void die(const char* msg);
void msg(const char* msg);
void print(const char* msg);
string to_lower(const string& str);
bool have_crlf(const char* buff, int sz);

// Socket helpers
int recv_exct(int fd, vector<char>& buff, size_t n);
int wrt_exct(int fd, const vector<char>& buff, size_t n);
int recv1(int fd, vector<char>& buff);
int send1(int fd, const vector<char>& buff);
