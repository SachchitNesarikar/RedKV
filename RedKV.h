#pragma once

#include <string>
#include <mutex>
#include <shared_mutex>
#include <vector>
#include <limits>
#include "store/HashTable.h"

class RedKV {
public:
    using Entry = ::Entry;

private:
    static RedKV* _instance;
    static std::mutex _instanceMutex;

    HashTable _store;
    mutable std::shared_mutex _storeMutex;

    RedKV();
    ~RedKV() = default;

public:
    static RedKV& getInstance();
    static void delInstance();

    void clear();

    void set(const std::string& key,
             const std::string& value,
             time_t expiry_epoch = std::numeric_limits<time_t>::max());

    void setWithExpiryFromNow(const std::string& key, const std::string& value, time_t expiry_from_now_sec);

    bool get(const std::string& key, std::string& value);
    bool del(const std::string& key);
    bool exists(const std::string& key);
    long long ttl(const std::string& key);

    std::vector<std::string> keys(const std::string& pattern);
    std::pair<size_t, std::vector<std::string>>
    scan(size_t cursor, const std::string& pattern, size_t count);

    bool is_loading() const;

    RedKV(const RedKV&) = delete;
    RedKV& operator=(const RedKV&) = delete;
};
