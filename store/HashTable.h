#pragma once
#include <string>
#include <vector>
#include <functional>
#include <ctime>

struct Entry {
    std::string v;
    std::time_t exp = 0;

    Entry() = default;
    Entry(const std::string& val, std::time_t expiry)
        : v(val), exp(expiry) {}
};

class HashTable {
public:
    explicit HashTable(size_t init_size = 16);

    bool set(const std::string& key, const Entry& e);
    bool get(const std::string& key, Entry& out);
    bool del(const std::string& key);
    bool exists(const std::string& key);

    size_t size() const;

    void rehash_step(size_t steps = 1);
    bool is_rehashing() const;

private:
    struct Node {
        std::string key;
        Entry value;
        Node* next;
    };

    std::vector<Node*> tab[2];
    size_t vis[2];
    long long ridx;

    size_t hash_key(const std::string& key) const;
    void maybe_start_rehash();
    Node* find_node(const std::string& key, size_t table);
};
