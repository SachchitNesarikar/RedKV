#include "Server.h"
#include "../cmnds/cmd.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <cerrno>

Server::Server(int port) : port(port), listen_fd(-1) { setup_listen_socket(); }

Server::~Server() {
    if (listen_fd >= 0) {
        close(listen_fd);
    }
}

void Server::setup_listen_socket() {
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        throw std::runtime_error("socket() failed");
    }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(listen_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        throw std::runtime_error("bind() failed");
    }

    if (listen(listen_fd, SOMAXCONN) < 0) {
        throw std::runtime_error("listen() failed");
    }

    make_nonblocking(listen_fd);
    std::cout << "Server listening on port " << port << std::endl;
}

void Server::make_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        throw std::runtime_error("fcntl(F_GETFL) failed");
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        throw std::runtime_error("fcntl(F_SETFL) failed");
    }
}

void Server::run() {
    event_loop.add_fd(listen_fd, EventLoop::EVENT_READABLE,
        [this](int fd, uint32_t events) {
            handle_accept(fd, events);
        });

    std::cout << "Event loop starting..." << std::endl;
    event_loop.run();
}

void Server::stop() {
    event_loop.stop();
}

void Server::handle_accept(int fd, uint32_t events) {
    (void)fd;
    (void)events;

    while (true) {
        sockaddr_in client_addr{};
        socklen_t addr_len = sizeof(client_addr);

        int client_fd = accept(listen_fd, (sockaddr*)&client_addr, &addr_len);

        if (client_fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            else if (errno == EINTR) continue;
            else {
                std::cerr << "accept() error: " << strerror(errno) << std::endl;
                break;
            }
        }

        make_nonblocking(client_fd);

        int opt = 1;
        setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));

        auto conn = std::make_unique<Connection>(client_fd);

        event_loop.add_fd(client_fd, EventLoop::EVENT_READABLE,
            [this](int fd, uint32_t events) {
                handle_client(fd, events);
            });

        connections[client_fd] = std::move(conn);

        std::cout << "New connection: fd=" << client_fd << ", total=" << connections.size() << std::endl;
    }
}

void Server::handle_client(int fd, uint32_t events) {
    auto it = connections.find(fd);
    if (it == connections.end()) {
        return;
    }

    Connection* conn = it->second.get();

    if (events & EventLoop::EVENT_ERROR) {
        close_connection(fd);
        return;
    }

    if (events & EventLoop::EVENT_READABLE) {
        if (!conn->try_read()) {
            close_connection(fd);
            return;
        }
        process_requests(conn);
    }

    if (events & EventLoop::EVENT_WRITABLE) {
        if (!conn->try_write()) {
            close_connection(fd);
            return;
        }

        if (!conn->has_pending_writes()) {
            event_loop.mod_fd(fd, EventLoop::EVENT_READABLE);
        }
    }
}

void Server::process_requests(Connection* conn) {
    while (conn->has_complete_request()) {
        auto cmd = conn->get_next_request();

        if (cmd.empty()) {
            continue;
        }

        std::string response = execute_command(cmd);
        conn->queue_response(response);
        event_loop.mod_fd(conn->get_fd(), EventLoop::EVENT_READABLE | EventLoop::EVENT_WRITABLE);
    }
}

std::string Server::execute_command(const std::vector<std::string>& cmd) {
    return cmd::execute(cmd, kv_store);
}

void Server::close_connection(int fd) {
    std::cout << "Closing connection: fd=" << fd << std::endl;

    event_loop.del_fd(fd);
    connections.erase(fd);
}
