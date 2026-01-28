#include "AOF.h"
#include "RESPParser.h"
#include "RedKV.h"
#include "utils.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cerrno>
#include <cstring>
#include <iostream>

AOF::AOF() = default; 
AOF::~AOF() = default;

static const char* AOF_FILENAME = "appendonly.aof";

AOF& AOF::instance() {
    static AOF inst;
    return inst;
}

void AOF::load() {
    int fd = open(AOF_FILENAME, O_RDONLY);
    if (fd < 0) return;

    {
        std::lock_guard<std::mutex> lg(tm);
        loading = true;
    }

    RESPParser parser(fd);
    while (true) {
        try {
            std::vector<std::string> req = parser.read_new_req();
            if (req.empty()) break;

            req[0] = to_lower(req[0]);

            if (req[0] == "set") {
                if (req.size() >= 3) {
                    time_t expiry = std::numeric_limits<time_t>::max();
                    RedKV::getInstance().set(req[1], req[2], expiry);
                }
            } else if (req[0] == "del") {
                for (size_t i = 1; i < req.size(); ++i)
                    RedKV::getInstance().del(req[i]);
            }
        } catch (const std::runtime_error&) {
            break;
        }
    }

    close(fd);

    {
        std::lock_guard<std::mutex> lg(tm);
        loading = false;
    }
}

void AOF::append(const std::vector<std::string>& cmd) {
    std::string out;
    out.reserve(256);

    out += "*" + std::to_string(cmd.size()) + "\r\n";
    for (const auto& s : cmd) {
        out += "$" + std::to_string(s.size()) + "\r\n";
        out += s;
        out += "\r\n";
    }

    int fd = open(AOF_FILENAME, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) {
        std::cerr << "AOF append: open failed: " << std::strerror(errno) << "\n";
        return;
    }

    ssize_t written = 0;
    const char* buf = out.data();
    size_t to_write = out.size();
    while (to_write > 0) {
        ssize_t w = ::write(fd, buf + written, to_write);
        if (w < 0) {
            if (errno == EINTR) continue;
            std::cerr << "AOF append: write failed: " << std::strerror(errno) << "\n";
            break;
        }
        written += w;
        to_write -= w;
    }

    if (fsync(fd) < 0) {
        std::cerr << "AOF append: fsync failed: " << std::strerror(errno) << "\n";
    }

    close(fd);
}
