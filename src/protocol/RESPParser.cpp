#include "RESPParser.h"
#include <sstream>

std::string RESPSerializer::simple_string(const std::string& s) {
    return "+" + s + "\r\n";
}

std::string RESPSerializer::error(const std::string& msg) {
    return "-" + msg + "\r\n";
}

std::string RESPSerializer::integer(long long n) {
    return ":" + std::to_string(n) + "\r\n";
}

std::string RESPSerializer::bulk_string(const std::string& s) {
    return "$" + std::to_string(s.size()) + "\r\n" + s + "\r\n";
}

std::string RESPSerializer::null_bulk_string() {
    return "$-1\r\n";
}

std::string RESPSerializer::array(const std::vector<std::string>& elems) {
    std::ostringstream oss;
    oss << "*" << elems.size() << "\r\n";
    for (const auto& x : elems) oss << bulk_string(x);
    return oss.str();
}