#pragma once

#include "EventLoop.h"
#include "Connection.h"
#include "../store/RedKV.h"
#include <memory>
#include <unordered_map>
#include "../cmnds/cmd.h"


class Server {
public:
    Server(int port);
    ~Server();

    void run();
    void stop();

private:
    int port;
    int listen_fd;
    EventLoop event_loop;
    RedKV kv_store;

    std::unordered_map<int, std::unique_ptr<Connection>> connections;

    void handle_accept(int fd, uint32_t events);
    void handle_client(int fd, uint32_t events);

    void accept_new_connection();
    void close_connection(int fd);
    void process_requests(Connection* conn);

    std::string execute_command(const std::vector<std::string>& cmd);

    void setup_listen_socket();
    void make_nonblocking(int fd);
};
