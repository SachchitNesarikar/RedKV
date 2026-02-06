#include "HashTable.h"
#include <functional>

HashTable::HashTable(size_t init_size) : tbl(init_size, nullptr), cnt(0) {}

HashTable::~HashTable() {
    for (auto* head : tbl) {
        while (head) {
            Node* next = head->next;
            delete head;
            head = next;
        }
    }
}

size_t HashTable::hash(const std::string& key) const {
    return std::hash<std::string>{}(key) % tbl.size();
}

HashTable::Node* HashTable::find_node(const std::string& key, size_t& bckt) {
    bckt = hash(key);
    Node* node = tbl[bckt];
    
    while (node) {
        if (node->key == key) return node;
        node = node->next;
    }
    
    return nullptr;
}

bool HashTable::set(const std::string& key, const Entry& entry) {
    size_t bckt;
    Node* node = find_node(key, bckt);
    
    if (node) {
        node->val = entry;
        return false;
    }
    
    // Insert new
    Node* new_node = new Node{key, entry, tbl[bckt]};
    tbl[bckt] = new_node;
    cnt++;
    
    return true;
}

bool HashTable::get(const std::string& key, Entry& out) {
    size_t bckt;
    Node* node = find_node(key, bckt);
    
    if (node) {
        out = node->val;
        return true;
    }
    
    return false;
}

bool HashTable::del(const std::string& key) {
    size_t bckt = hash(key);
    Node** indirect = &tbl[bckt];
    
    while (*indirect) {
        Node* node = *indirect;
        if (node->key == key) {
            *indirect = node->next;
            delete node;
            cnt--;
            return true;
        }
        indirect = &node->next;
    }
    
    return false;
}

bool HashTable::exists(const std::string& key) {
    Entry dummy;
    return get(key, dummy);
}