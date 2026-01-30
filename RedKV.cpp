#include "RedKV.h"
#include "AOF.h"
#include <ctime>

RedKV* RedKV::_instance = nullptr;
std::mutex RedKV::_instanceMutex;

RedKV& RedKV::getInstance() {
    std::lock_guard<std::mutex> lock(_instanceMutex);
    if (!_instance)
        _instance = new RedKV();
    return *_instance;
}

void RedKV::delInstance() {
    std::lock_guard<std::mutex> lock(_instanceMutex);
    delete _instance;
    _instance = nullptr;
}

void RedKV::clear() {
    std::unique_lock<std::shared_mutex> lock(_storeMutex);
    _store = HashTable();
}

void RedKV::set(const std::string& key,
                const std::string& value,
                time_t expiry_epoch) {
    std::unique_lock<std::shared_mutex> lock(_storeMutex);
    _store.set(key, Entry{value, expiry_epoch});
}

void RedKV::setWithExpiryFromNow(const std::string& key,
                                 const std::string& value,
                                 time_t expiry_from_now_sec) {
    set(key, value, time(nullptr) + expiry_from_now_sec);
}

bool RedKV::get(const std::string& key, std::string& value) {
    std::shared_lock<std::shared_mutex> lock(_storeMutex);
    Entry e;
    if (!_store.get(key, e)) return false;
    if (e.exp < time(nullptr)) return false;
    value = e.v;
    return true;
}

bool RedKV::del(const std::string& key) {
    std::unique_lock<std::shared_mutex> lock(_storeMutex);
    return _store.del(key);
}

bool RedKV::exists(const std::string& key) {
    std::shared_lock<std::shared_mutex> lock(_storeMutex);
    Entry e;
    if (!_store.get(key, e)) return false;
    return e.exp >= time(nullptr);
}

long long RedKV::ttl(const std::string& key) {
    std::shared_lock<std::shared_mutex> lock(_storeMutex);
    Entry e;
    if (!_store.get(key, e)) return -2;

    time_t now = time(nullptr);
    if (e.exp < now) return -2;
    if (e.exp == std::numeric_limits<time_t>::max()) return -1;
    return e.exp - now;
}

bool RedKV::is_loading() const {
    return AOF::instance().is_loading();
}
