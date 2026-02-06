#include "Connection.h"
#include <unistd.h>
#include <sys/socket.h>
#include <cerrno>
#include <cstring>
#include <sys/time.h>
#include <cstdint>
#include <chrono>

static uint64_t get_time_ms() {
    using namespace std::chrono;
    return static_cast<uint64_t>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

Connection::Connection(int fd)
    : fd(fd)
    , state(STATE_READING_HEADER)
    , read_buff()
    , read_pos(0)
    , wq()
    , curr_wrt()
    , wp(0)
    , resp_st(RESP_WAIT_ARRAY)
    , exp_args(0)
    , curr_arg(0)
    , curr_bulk_size(0)
    , curr_cmd()
    , comp_cmds()
    , create_tym(get_time_ms())
    , req_proc(0)
{
}

Connection::~Connection() {
    if (fd >= 0) {
        close(fd);
    }
}

bool Connection::try_read() {
    const size_t BUFFER_SIZE = 16384;
    char buffer[BUFFER_SIZE];

    while (true) {
        ssize_t n = recv(fd, buffer, BUFFER_SIZE, 0);

        if (n > 0) {
            read_buff.append(buffer, static_cast<size_t>(n));

            if (!parse_resp()) {
                mark_closed();
                return false;
            }
        } else if (n == 0) {
            mark_closed();
            return false;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return true;
            else if (errno == EINTR) continue;
            else {
                mark_closed();
                return false;
            }
        }
    }
}

bool Connection::try_write() {
    while (!wq.empty() || !curr_wrt.empty()) {

        if (curr_wrt.empty()) {
            curr_wrt = std::move(wq.front());
            wq.pop_front();
            wp = 0;
        }

        ssize_t n = send(fd, curr_wrt.data() + wp, curr_wrt.size() - wp, MSG_NOSIGNAL);

        if (n > 0) {
            wp += static_cast<size_t>(n);
            if (wp >= curr_wrt.size()) {
                curr_wrt.clear();
                wp = 0;
            }
        } else if (n == 0) {
            mark_closed();
            return false;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return true;
            } else if (errno == EINTR) {
                continue;
            } else {
                mark_closed();
                return false;
            }
        }
    }

    return true;
}

bool Connection::parse_resp() {
    size_t pos = read_pos;

    while (pos < read_buff.size()) {
        switch (resp_st) {
            case RESP_WAIT_ARRAY: {
                if (read_buff[pos] != '*') {
                    return false;
                }

                size_t crlf = read_buff.find("\r\n", pos);
                if (crlf == std::string::npos) {
                    read_pos = pos;
                    return true;
                }

                std::string num_str = read_buff.substr(pos + 1, crlf - pos - 1);
                exp_args = std::stoi(num_str);

                if (exp_args < 0 || exp_args > 1000000) {
                    return false;
                }

                curr_arg = 0;
                curr_cmd.clear();
                curr_cmd.reserve(static_cast<size_t>(exp_args));

                pos = crlf + 2;
                resp_st = RESP_WAIT_BULK_SIZE;
                break;
            }

            case RESP_WAIT_BULK_SIZE: {
                if (read_buff[pos] != '$') {
                    return false;
                }

                size_t crlf = read_buff.find("\r\n", pos);
                if (crlf == std::string::npos) {
                    read_pos = pos;
                    return true;
                }

                std::string size_str = read_buff.substr(pos + 1, crlf - pos - 1);
                curr_bulk_size = std::stoi(size_str);

                if (curr_bulk_size < 0 || curr_bulk_size > 512 * 1024 * 1024) {
                    return false;
                }

                pos = crlf + 2;
                resp_st = RESP_WAIT_BULK_DATA;
                break;
            }

            case RESP_WAIT_BULK_DATA: {
                if (pos + static_cast<size_t>(curr_bulk_size) + 2 > read_buff.size()) {
                    read_pos = pos;
                    return true;
                }

                std::string arg = read_buff.substr(pos, static_cast<size_t>(curr_bulk_size));
                curr_cmd.push_back(std::move(arg));
                curr_arg++;

                pos += static_cast<size_t>(curr_bulk_size) + 2;

                if (curr_arg >= exp_args) {
                    comp_cmds.push_back(std::move(curr_cmd));
                    curr_cmd.clear();
                    resp_st = RESP_WAIT_ARRAY;
                } else {
                    resp_st = RESP_WAIT_BULK_SIZE;
                }
                break;
            }
        }
    }

    if (pos > 0) {
        read_buff.erase(0, pos);
        read_pos = 0;
    }

    return true;
}

bool Connection::has_complete_request() {
    return !comp_cmds.empty();
}

std::vector<std::string> Connection::get_next_request() {
    if (comp_cmds.empty()) {
        return {};
    }

    auto cmd = std::move(comp_cmds.front());
    comp_cmds.pop_front();
    req_proc++;
    return cmd;
}

void Connection::queue_response(const std::string& response) {
    wq.push_back(response);
}

bool Connection::has_pending_writes() const {
    return !wq.empty() || !curr_wrt.empty();
}

void Connection::mark_closed() {
    state = STATE_CLOSED;
}
