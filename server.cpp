#include "utils.h"
#include "RESPParser.h"
#include "RedSrvr.h"
#include "cmd.h"
#include "RedKV.h"
#include "AOF.h"

#include <atomic>
#include <csignal>
#include <thread>
#include <iostream>
#include <vector>
#include <memory>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

static atomic<bool> server_running{true};

#define PORT 2000

void handle_sigint(int) {
    server_running = false;
}

void process_request(vector<string>& req, int fd) {
    if (req.empty()) return;

    if (req[0] == "command") {
        const string resp = "*0\r\n";  // legal, minimal
        vector<char> buf(resp.begin(), resp.end());
        wrt_exct(fd, buf, buf.size());
        return;
    }

    red_ftype fn = get_redis_func(req[0]);
    if (!fn) {
        redis::Error err("ERR unknown command");
        string r = err.serialize();
        vector<char> buf(r.begin(), r.end());
        wrt_exct(fd, buf, buf.size());
        return;
    }

    unique_ptr<redis::RedisObject> obj = fn(req);
    if (!obj) {
        redis::Error err("ERR internal error");
        string r = err.serialize();
        vector<char> buf(r.begin(), r.end());
        wrt_exct(fd, buf, buf.size());
        return;
    }

    string r = obj->serialize();
    vector<char> buf(r.begin(), r.end());
    wrt_exct(fd, buf, buf.size());
}

void handle_client(int client_fd) {
    RESPParser parser(client_fd);

    while (true) {
        try {
            vector<string> req = parser.read_new_req();
            if (req.empty()) break;

            req[0] = to_lower(req[0]);
            process_request(req, client_fd);

        } catch (const runtime_error&) {
            break;
        }
    }

    close(client_fd);
}

int setup_server() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) die("socket");

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(fd, (sockaddr*)&addr, sizeof(addr)) < 0)
        die("bind");

    if (listen(fd, SOMAXCONN) < 0)
        die("listen");

    cout << "Server listening on port: " << PORT << endl;
    return fd;
}

void dispatch_clients(int server_fd) {
    while (server_running) {
        sockaddr_in caddr{};
        socklen_t len = sizeof(caddr);

        int cfd = accept(server_fd, (sockaddr*)&caddr, &len);
        if (cfd < 0) continue;

        thread(handle_client, cfd).detach();
    }
}

int main() {
    signal(SIGINT, handle_sigint);

    AOF::instance().load();

    int server_fd = setup_server();
    dispatch_clients(server_fd);

    close(server_fd);
    RedKV::delInstance();
}
