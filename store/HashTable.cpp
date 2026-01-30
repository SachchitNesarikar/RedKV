#include "HashTable.h"
#include <functional>

static constexpr double LOAD_FACTOR = 1.0;

HashTable::HashTable(size_t initSZ)
    : ridx(-1)
{
    tab[0].resize(initSZ, nullptr);
    tab[1].clear();
    vis[0] = vis[1] = 0;
}

size_t HashTable::hash_key(const std::string& key) const {
    return std::hash<std::string>{}(key);
}

bool HashTable::is_rehashing() const {
    return ridx != -1;
}

void HashTable::maybe_start_rehash() {
    if (is_rehashing()) return;
    if (vis[0] >= tab[0].size() * LOAD_FACTOR) {
        tab[1].resize(tab[0].size() * 2, nullptr);
        ridx = 0;
    }
}

void HashTable::rehash_step(size_t steps) {
    if (!is_rehashing()) return;

    while (steps-- && ridx < (long long)tab[0].size()) {
        Node* n = tab[0][ridx];
        while (n) {
            Node* next = n->next;
            size_t h = hash_key(n->key) % tab[1].size();
            n->next = tab[1][h];
            tab[1][h] = n;
            vis[1]++;
            vis[0]--;
            n = next;
        }
        tab[0][ridx++] = nullptr;
    }

    if (ridx >= (long long)tab[0].size()) {
        tab[0].swap(tab[1]);
        tab[1].clear();
        vis[0] = vis[1];
        vis[1] = 0;
        ridx = -1;
    }
}

HashTable::Node* HashTable::find_node(const std::string& key, size_t t) {
    size_t h = hash_key(key) % tab[t].size();
    Node* n = tab[t][h];
    while (n) {
        if (n->key == key) return n;
        n = n->next;
    }
    return nullptr;
}

bool HashTable::set(const std::string& key, const Entry& e) {
    rehash_step();
    maybe_start_rehash();

    size_t t = is_rehashing() ? 1 : 0;
    size_t h = hash_key(key) % tab[t].size();

    for (Node* n = tab[t][h]; n; n = n->next) {
        if (n->key == key) {
            n->value = e;
            return false;
        }
    }

    tab[t][h] = new Node{key, e, tab[t][h]};
    vis[t]++;
    return true;
}

bool HashTable::get(const std::string& key, Entry& out) {
    rehash_step();
    for (int t = 0; t <= 1; t++) {
        if (t == 1 && !is_rehashing()) break;
        if (Node* n = find_node(key, t)) {
            out = n->value;
            return true;
        }
    }
    return false;
}

bool HashTable::del(const std::string& key) {
    rehash_step();
    for (int t = 0; t <= 1; t++) {
        if (t == 1 && !is_rehashing()) break;

        size_t h = hash_key(key) % tab[t].size();
        Node* prev = nullptr;
        Node* cur = tab[t][h];

        while (cur) {
            if (cur->key == key) {
                if (prev) prev->next = cur->next;
                else tab[t][h] = cur->next;
                delete cur;
                vis[t]--;
                return true;
            }
            prev = cur;
            cur = cur->next;
        }
    }
    return false;
}

bool HashTable::exists(const std::string& key) {
    Entry tmp;
    return get(key, tmp);
}

size_t HashTable::size() const {
    return vis[0] + vis[1];
}
