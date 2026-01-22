#pragma once
#include <string>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <limits>
#include <chrono>
#include <memory>

using namespace std;

/*
Singleton in-memory key-value store
Supports optional expiry timestamps
*/
class RedKV {
public:
    struct Entry {
        string v;      // value
        time_t exp;    // expiry epoch (seconds since epoch)
    };

private:
    // Singleton instance
    static RedKV* _instance;
    static mutex _instanceMutex;

    // Actual key-value store
    unordered_map<string, Entry> _data;
    mutable shared_mutex _storeMutex;

    // Private constructor/destructor
    RedKV() = default;
    ~RedKV() = default;

public:
    // Get singleton instance
    static RedKV& getInstance();

    // Delete singleton instance and clear data
    static void delInstance();

    // Clear all keys
    void clear();

    // Set key with absolute expiry epoch (default: never expires)
    void set(const string& key, const string& value, time_t expiry_epoch = numeric_limits<time_t>::max());

    // Set key with expiry relative to now
    void setWithExpiryFromNow(const string& key, const string& value, time_t expiry_from_now_sec);

    // Get key; returns false if key not found or expired
    bool get(const string& key, string& value);

    // Delete a key; returns true if key existed and was removed, false otherwise
    bool del(const string& key);

    bool exists(const std::string& key);

    long long ttl(const std::string& key);

    // Disable copy and assignment
    RedKV(const RedKV&) = delete;
    RedKV& operator=(const RedKV&) = delete;
};
