#include "RedKV.h"

using namespace std;

// Initialize static member variables
RedKV* RedKV::_instance = nullptr;
mutex RedKV::_instanceMutex;

// Get singleton instance
RedKV& RedKV::getInstance() {
    // Lock the instance mutex to ensure thread safety
    lock_guard<mutex> lock(_instanceMutex);
    if (!_instance) {
        _instance = new RedKV();
    }
    return *_instance;
}

// Delete singleton instance and clear data
void RedKV::delInstance() {
    lock_guard<mutex> lock(_instanceMutex);
    if (_instance) {
        delete _instance;
        _instance = nullptr;
    }
}

// Clear all keys
void RedKV::clear() {
    lock_guard<shared_mutex> lock(_storeMutex);
    _data.clear();
}

// Set key with absolute expiry epoch (default: never expires)
void RedKV::set(const string& key, const string& value, time_t expiry_epoch) {
    lock_guard<shared_mutex> lock(_storeMutex);
    _data[key] = {value, expiry_epoch};
}

// Set key with expiry relative to now
void RedKV::setWithExpiryFromNow(const string& key, const string& value, time_t expiry_from_now_sec) {
    time_t expiry_epoch = time(nullptr) + expiry_from_now_sec;
    set(key, value, expiry_epoch);
}

// Get key; returns false if key not found or expired
bool RedKV::get(const string& key, string& value) {
    shared_lock<shared_mutex> lock(_storeMutex);
    auto it = _data.find(key);
    if (it == _data.end()) {
        return false;  // Key not found
    }

    // Check if the key has expired
    if (it->second.exp < time(nullptr)) {
        _data.erase(it);  // Remove expired key
        return false;
    }

    // Key found and not expired
    value = it->second.v;
    return true;
}
