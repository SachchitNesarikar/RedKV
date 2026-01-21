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

bool RedKV::get(const string& key, string& value) {
    // Phase 1: shared read
    shared_lock<shared_mutex> rlock(_storeMutex);
    auto it_read = _data.find(key);
    if (it_read == _data.end()) return false;

    if (it_read->second.exp >= time(nullptr)) {
        value = it_read->second.v;
        return true;
    }

    // Phase 2: exclusive erase
    unique_lock<shared_mutex> wlock(_storeMutex);
    auto it_write = _data.find(key);
    if (it_write != _data.end() && it_write->second.exp < time(nullptr)) {
        _data.erase(it_write);
    }

    return false;
}


