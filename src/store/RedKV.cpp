#include "RedKV.h"
#include <ctime>

RedKV::RedKV() : store(1024) {}

time_t RedKV::current_time() const {
    return time(nullptr);
}

void RedKV::set(const std::string& key, const std::string& val) {
    Entry entry(val, 0);  
    store.set(key, entry);
}

void RedKV::set(const std::string& key, const std::string& val, time_t expiry_seconds) {
    time_t expiry_time = current_time() + expiry_seconds;
    Entry entry(val, expiry_time);
    store.set(key, entry);
}

bool RedKV::get(const std::string& key, std::string& val) {
    Entry entry;
    if (!store.get(key, entry)) return false;
    
    if (entry.is_expired(current_time())) {
        store.del(key);
        return false;
    }
    
    val = entry.val;
    return true;
}

bool RedKV::del(const std::string& key) {
    return store.del(key);
}

bool RedKV::exists(const std::string& key) {
    Entry entry;
    if (!store.get(key, entry)) return false;
    
    if (entry.is_expired(current_time())) {
        store.del(key);
        return false;
    }
    
    return true;
}

long long RedKV::ttl(const std::string& key) {
    Entry entry;
    if (!store.get(key, entry)) return -2;  
    
    time_t now = current_time();
    
    if (entry.is_expired(now)) {
        store.del(key);
        return -2;
    }
    
    if (entry.exp == 0) return -1;
    return entry.exp - now;
}