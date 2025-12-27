#pragma once

#include <string>
#include <vector>
#include <memory>
#include <stdexcept>

namespace redis {

// ---------- Base Redis object ----------
class RedisObject {
public:
    virtual ~RedisObject() = default;
    virtual std::string serialize() const = 0;
};

// ---------- +OK / +PONG ----------
class SimpleString : public RedisObject {
    std::string value;
public:
    explicit SimpleString(std::string v) : value(std::move(v)) {}
    std::string serialize() const override {
        return "+" + value + "\r\n";
    }
};

// ---------- -ERR ----------
class Error : public RedisObject {
    std::string msg;
public:
    explicit Error(std::string m) : msg(std::move(m)) {}
    std::string serialize() const override {
        return "-" + msg + "\r\n";
    }
};

// ---------- $bulk ----------
class BulkString : public RedisObject {
    std::string value;
public:
    explicit BulkString(std::string v) : value(std::move(v)) {}
    std::string serialize() const override {
        return "$" + std::to_string(value.size()) + "\r\n" +
               value + "\r\n";
    }
};

// ---------- $-1 ----------
class NullString : public RedisObject {
public:
    std::string serialize() const override {
        return "$-1\r\n";
    }
};

// ---------- *array ----------
class Array : public RedisObject {
    std::vector<std::unique_ptr<RedisObject>> elems;
public:
    void add_element(std::unique_ptr<RedisObject> e) {
        elems.push_back(std::move(e));
    }

    std::string serialize() const override {
        std::string out = "*" + std::to_string(elems.size()) + "\r\n";
        for (const auto& e : elems)
            out += e->serialize();
        return out;
    }
};

} // namespace redis

// ---------- Server error ----------
class RedisServerError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};
