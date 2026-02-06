#pragma once

#include <unistd.h>
#include <string>
#include <vector>
#include <deque>
#include <memory>
#include <cstdint>

class Connection {
public:
    enum State {
        STATE_READING_HEADER,
        STATE_READING_BODY,
        STATE_PROCESSING,
        STATE_WRITING,
        STATE_CLOSED
    };

    explicit Connection(int fd);
    ~Connection();

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    int get_fd() const { return fd; }
    State get_state() const { return state; }

    bool try_read();
    bool try_write();

    bool has_complete_request();
    std::vector<std::string> get_next_request();

    void queue_response(const std::string& response);
    bool has_pending_writes() const;

    void mark_closed();
    bool is_closed() const { return state == STATE_CLOSED; }

    uint64_t get_created_time() const { return create_tym; }
    size_t get_requests_processed() const { return req_proc; }

private:
    int fd;
    State state;

    std::string read_buff;
    size_t read_pos;

    std::deque<std::string> wq;
    std::string curr_wrt;
    size_t wp;

    enum RESPState {
        RESP_WAIT_ARRAY,
        RESP_WAIT_BULK_SIZE,
        RESP_WAIT_BULK_DATA
    };

    RESPState resp_st;
    int exp_args;
    int curr_arg;
    int curr_bulk_size;
    std::vector<std::string> curr_cmd;
    std::deque<std::vector<std::string>> comp_cmds;

    uint64_t create_tym;
    size_t req_proc;

    void reset_resp_parser();
    bool parse_resp();
    ssize_t do_read();
    ssize_t do_write();
};
