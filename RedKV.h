#pragma once
#include <string>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <limits>
#include <chrono>
#include <memory>
#include <vector>

using namespace std;

class RedKV {
public:
    struct Entry {
        string v;
        time_t exp;
    };

private:
    static RedKV* _instance;
    static mutex _instanceMutex;

    unordered_map<string, Entry> _data;
    mutable shared_mutex _storeMutex;

    RedKV() = default;
    ~RedKV() = default;

public:
    static RedKV& getInstance();
    static void delInstance();
    void clear();
    void set(const string& key, const string& value, time_t expiry_epoch = numeric_limits<time_t>::max());
    void setWithExpiryFromNow(const string& key, const string& value, time_t expiry_from_now_sec);
    bool get(const string& key, string& value);
    bool del(const string& key);
    bool exists(const std::string& key);
    long long ttl(const std::string& key);
    std::vector<std::string> keys(const std::string& pattern);
    std::pair<size_t, std::vector<std::string>> scan(size_t cursor, const std::string& pattern, size_t count);
    bool is_loading() const;

    RedKV(const RedKV&) = delete;
    RedKV& operator=(const RedKV&) = delete;
};
