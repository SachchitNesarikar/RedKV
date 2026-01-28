#include "RedKV.h"
#include "AOF.h"
#include <atomic>
using namespace std;

static atomic<bool> _loading{false};

RedKV* RedKV::_instance = nullptr;
mutex RedKV::_instanceMutex;

RedKV& RedKV::getInstance() {
    lock_guard<mutex> lock(_instanceMutex);
    if (!_instance) {
        _instance = new RedKV();
    }
    return *_instance;
}

void RedKV::delInstance() {
    lock_guard<mutex> lock(_instanceMutex);
    if (_instance) {
        delete _instance;
        _instance = nullptr;
    }
}

void RedKV::clear() {
    lock_guard<shared_mutex> lock(_storeMutex);
    _data.clear();
}

void RedKV::set(const string& key, const string& value, time_t expiry_epoch) {
    lock_guard<shared_mutex> lock(_storeMutex);
    _data[key] = {value, expiry_epoch};
}

void RedKV::setWithExpiryFromNow(const string& key, const string& value, time_t expiry_from_now_sec) {
    time_t expiry_epoch = time(nullptr) + expiry_from_now_sec;
    set(key, value, expiry_epoch);
}

bool RedKV::get(const string& key, string& value) {
    shared_lock<shared_mutex> rlock(_storeMutex);
    auto it_read = _data.find(key);
    if (it_read == _data.end()) return false;

    if (it_read->second.exp >= time(nullptr)) {
        value = it_read->second.v;
        return true;
    }

    unique_lock<shared_mutex> wlock(_storeMutex);
    auto it_write = _data.find(key);
    if (it_write != _data.end() && it_write->second.exp < time(nullptr)) {
        _data.erase(it_write);
    }

    return false;
}

bool RedKV::del(const string& key) {
    unique_lock<shared_mutex> lock(_storeMutex);

    auto it = _data.find(key);
    if (it == _data.end()) return false;

    if (it->second.exp < time(nullptr)) {
        _data.erase(it);
        return false;
    }

    _data.erase(it);
    return true;
}

bool RedKV::exists(const string& key) {
    shared_lock<shared_mutex> lock(_storeMutex);

    auto it = _data.find(key);
    if (it == _data.end()) return false;

    if (it->second.exp < time(nullptr)) {
        return false;
    }

    return true;
}

long long RedKV::ttl(const string& key) {
    shared_lock<shared_mutex> lock(_storeMutex);

    auto it = _data.find(key);
    if (it == _data.end())
        return -2;

    time_t now = time(nullptr);

    if (it->second.exp < now)
        return -2;

    if (it->second.exp == numeric_limits<time_t>::max())
        return -1;

    return static_cast<long long>(it->second.exp - now);
}

bool mtch(const std::string& key, const std::string& pattern) {
    if (pattern == "*") return true;

    auto pos = pattern.find('*');
    if (pos == std::string::npos)
        return key == pattern;

    std::string pref = pattern.substr(0, pos);
    return key.compare(0, pref.size(), pref) == 0;
}

std::vector<std::string> RedKV::keys(const std::string& pattern) {
    std::vector<std::string> result;
    time_t now = time(nullptr);

    std::shared_lock<std::shared_mutex> lock(_storeMutex);

    for (const auto& [k, entry] : _data) {
        if (entry.exp < now)
            continue;

        if (mtch(k, pattern))
            result.push_back(k);
    }

    return result;
}

std::pair<size_t, std::vector<std::string>>
RedKV::scan(size_t cursor, const std::string& pattern, size_t cnt) {
    std::vector<std::string> out;
    time_t now = time(nullptr);

    size_t idx = 0;

    while (true) {
        std::shared_lock<std::shared_mutex> rlock(_storeMutex);

        auto it = _data.begin();
        std::advance(it, cursor);

        for (; it != _data.end(); ++it, ++idx) {
            if (idx < cursor) continue;

            if (it->second.exp < now) {
                std::string dead = it->first;
                rlock.unlock();

                std::unique_lock<std::shared_mutex> wlock(_storeMutex);
                auto wit = _data.find(dead);
                if (wit != _data.end() && wit->second.exp < now)
                    _data.erase(wit);

                goto restart;
            }

            if (!mtch(it->first, pattern)) continue;

            out.push_back(it->first);
            if (out.size() >= cnt)
                return {idx + 1, out};
        }

        break;

    restart:
        idx = cursor;
        continue;
    }

    return {0, out};
}

bool RedKV::is_loading() const {
    return AOF::instance().is_loading();
}
