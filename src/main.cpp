#include "server/Server.h"
#include <iostream>
#include <csignal>
#include <cstdlib>

static Server* g_server = nullptr;

void signal_handler(int signum) {
    std::cout << "\nReceived signal " << signum << ", shutting down..." << std::endl;
    if (g_server) {
        g_server->stop();
    }
}

int main(int argc, char* argv[]) {
    int port = 6379;
    
    if (argc > 1) {
        port = std::atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            std::cerr << "Invalid port number" << std::endl;
            return 1;
        }
    }
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGPIPE, SIG_IGN);
    
    try {
        Server server(port);
        g_server = &server;
        
        std::cout << "Redis-like server starting on port " << port << std::endl;
        server.run();
        
        std::cout << "Server shut down cleanly" << std::endl;
        
    } 
    
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}