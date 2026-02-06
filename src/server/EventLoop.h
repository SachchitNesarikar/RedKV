#pragma once

#include <sys/epoll.h>
#include <functional>
#include <unordered_map>
#include <vector>
#include <cstdint>

class EventLoop {
public:
    static constexpr int MAX_EVENTS = 10000;
    
    enum EventType {
        EVENT_READABLE = EPOLLIN,
        EVENT_WRITABLE = EPOLLOUT,
        EVENT_ERROR = EPOLLERR | EPOLLHUP
    };

    using EventHandler = std::function<void(int fd, uint32_t events)>;

    EventLoop();
    ~EventLoop();

    EventLoop(const EventLoop&) = delete;
    EventLoop& operator=(const EventLoop&) = delete;

    bool add_fd(int fd, uint32_t events, EventHandler handler);
    bool mod_fd(int fd, uint32_t events);
    bool del_fd(int fd);

    void run();
    void stop();

    uint64_t get_current_time_ms() const;

private:
    int epoll_fd;
    bool running;
    
    struct FDHandler {
        EventHandler handler;
        uint32_t events;
    };
    
    std::unordered_map<int, FDHandler> handlers;
    
    void process_events(int timeout_ms);
};