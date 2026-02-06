#pragma once

#include <string>
#include <vector>
#include <ctime>

struct Entry {
    std::string val;
    time_t exp;
    
    Entry() : exp(0) {}
    Entry(const std::string& v, time_t exp) : val(v), exp(exp) {}
    
    bool is_expired(time_t now) const {
        return exp > 0 && exp < now;
    }
};

class HashTable {
public:
    explicit HashTable(size_t init_size = 16);
    ~HashTable();

    bool set(const std::string& key, const Entry& entry);
    bool get(const std::string& key, Entry& out);
    bool del(const std::string& key);
    bool exists(const std::string& key);
    
    size_t size() const { return cnt; }

private:
    struct Node {
        std::string key;
        Entry val;
        Node* next;
    };

    std::vector<Node*> tbl;
    size_t cnt;
    
    size_t hash(const std::string& key) const;
    Node* find_node(const std::string& key, size_t& bckt);
};