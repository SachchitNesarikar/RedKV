#pragma once

#include "HashTable.h"
#include <string>
#include <limits>

class RedKV {
public:
    RedKV();
    ~RedKV() = default;

    void set(const std::string& key, const std::string& value);
    void set(const std::string& key, const std::string& value, time_t expiry_seconds);
    
    bool get(const std::string& key, std::string& value);
    bool del(const std::string& key);
    bool exists(const std::string& key);
    
    long long ttl(const std::string& key);
    
    size_t dbsize() const { return store.size(); }

private:
    HashTable store;
    
    time_t current_time() const;
};