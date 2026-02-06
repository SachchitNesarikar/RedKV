#include "EventLoop.h"
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <sys/time.h>
#include <iostream>

EventLoop::EventLoop() : running(false) {
    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) throw std::runtime_error("epoll_create1 failed");
}

EventLoop::~EventLoop() {
    if (epoll_fd >= 0) {
        close(epoll_fd);
    }
}

bool EventLoop::add_fd(int fd, uint32_t events, EventHandler handler) {
    epoll_event ev{};
    ev.events = events | EPOLLET;  
    ev.data.fd = fd;
    
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) < 0) {
        return false;
    }
    
    handlers[fd] = FDHandler{std::move(handler), events};
    return true;
}

bool EventLoop::mod_fd(int fd, uint32_t events) {
    auto it = handlers.find(fd);
    if (it == handlers.end()) {
        return false;
    }
    
    epoll_event ev{};
    ev.events = events | EPOLLET;
    ev.data.fd = fd;
    
    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev) < 0) {
        return false;
    }
    
    it->second.events = events;
    return true;
}

bool EventLoop::del_fd(int fd) {
    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr) < 0) {
        return false;
    }
    
    handlers.erase(fd);
    return true;
}

void EventLoop::run() {
    running = true;
    
    while (running) process_events(100); 
}

void EventLoop::stop() {
    running = false;
}

void EventLoop::process_events(int timeout_ms) {
    epoll_event events[MAX_EVENTS];
    int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, timeout_ms);
    if (nfds < 0) {
        if (errno != EINTR) std::cerr << "epoll_wait error: " << strerror(errno) << std::endl;
        return;
    }
    
    for (int i = 0; i < nfds; ++i) {
        int fd = events[i].data.fd;
        uint32_t ev = events[i].events;
        auto it = handlers.find(fd);
        if (it != handlers.end()) it->second.handler(fd, ev);
    }
}

uint64_t EventLoop::get_current_time_ms() const {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return static_cast<uint64_t>(tv.tv_sec) * 1000 + tv.tv_usec / 1000;
}