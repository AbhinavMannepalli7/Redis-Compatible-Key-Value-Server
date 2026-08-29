#include "socket.hpp"
#include "connection.hpp"
#include "store.hpp"

#include <stdexcept>
#include <unistd.h>
#include <unordered_map>
#include <memory>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <thread>
#include <vector>
#include <cstdio>
#include <cerrno>   

typedef long long ll;
#define PORT 4950
#define BACKLOG 100
#define MAX_EVENTS 64

void set_epoll_events(int epoll_fd, int fd, uint32_t events) {
    struct epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev);
}

void handle_new_connections(int listen_fd, int epoll_fd, std::unordered_map<int, std::unique_ptr<Connection>>& connections) {
    while (true) {
        // accept4 with SOCK_NONBLOCK sets non-blocking atomically at accept time
        int client_fd = accept4(listen_fd, nullptr, nullptr, SOCK_NONBLOCK);
        if (client_fd == -1) {
            // no more pending connections right now
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            // transient errors: this particular accept failed, but there may
            // still be more pending connections in the queue, so keep going
            if (errno == ECONNABORTED || errno == EINTR) continue;
            perror("accept4");
            break;
        }

        struct epoll_event ev{};
        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = client_fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);
        connections[client_fd] = std::make_unique<Connection>(client_fd);
    }
}

void worker_loop(int thread_id, Store& store) {
    (void)thread_id;

    struct sockaddr_in addrinfo;
    addrinfo.sin_family = AF_INET;
    addrinfo.sin_port = htons(PORT);
    addrinfo.sin_addr.s_addr = htonl(INADDR_ANY);

    Socket listener(AF_INET, SOCK_STREAM, 0);
    Socket::set_nonblocking(listener.fd());

    int opt = 1;
    if (setsockopt(listener.fd(), SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) == -1) {
        perror("setsockopt");
    }

    listener.bindSocket(addrinfo);
    listener.startListening(BACKLOG);

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) { perror("epoll_create1"); exit(1); }

    struct epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = listener.fd();
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listener.fd(), &ev) == -1) {
        perror("epoll_ctl: listen_fd");
        exit(1);
    }

    struct epoll_event events[MAX_EVENTS];
    std::unordered_map<int, std::unique_ptr<Connection>> connections;

    int timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);

    struct epoll_event timer_ev{};
    timer_ev.events = EPOLLIN;
    timer_ev.data.fd = timer_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, timer_fd, &timer_ev);

    while (true) {
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (n == -1) { perror("epoll_wait"); break; }

        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;

            if (fd == listener.fd()) {
                handle_new_connections(listener.fd(), epoll_fd, connections);
                continue;
            }

            if (fd == timer_fd) {
                uint64_t expirations;
                read(timer_fd, &expirations, sizeof(expirations));
                store.handle_expirations();
                continue;
            }

            // existing client fd
            if (events[i].events & (EPOLLERR | EPOLLHUP)) {
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
                connections.erase(fd);
                continue;
            }

            auto it = connections.find(fd);
            if (it == connections.end()) {
                continue;
            }
            auto& conn = it->second;

            if (events[i].events & EPOLLIN) {
                if (!conn->do_read(store)) {
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
                    connections.erase(fd);
                    continue;
                }
                if (conn->has_data_to_write()) {
                    set_epoll_events(epoll_fd, fd, EPOLLIN | EPOLLOUT | EPOLLET);
                }
            }

            if (events[i].events & EPOLLOUT) {
                if (!conn->do_write()) {
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
                    connections.erase(fd);
                    continue;
                }
                if (!conn->has_data_to_write()) {
                    set_epoll_events(epoll_fd, fd, EPOLLIN | EPOLLET);
                }
            }
        }

        auto next = store.next_expiry();
        itimerspec timer{};
        if (next) {
            timer.it_value.tv_sec = *next / 1000;
            timer.it_value.tv_nsec = (*next % 1000) * 1000000;
            timerfd_settime(timer_fd, TFD_TIMER_ABSTIME, &timer, nullptr);
        } else {
            // disables the timer
            timerfd_settime(timer_fd, 0, &timer, nullptr);
        }
    }
}

int main() {
    Store store;

    int num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0) num_threads = 4; // hardware_concurrency() can return 0, so fall back

    std::vector<std::thread> workers;
    workers.reserve(num_threads);
    for (int i = 0; i < num_threads; i++) {
        workers.emplace_back(worker_loop, i, std::ref(store));
    }
    for (auto& t : workers) {
        t.join(); // blocks forever until worker_loop returns
    }
}

// g++-15 -std=c++23 -Wall main.cpp socket.cpp connection.cpp command.cpp dispatcher.cpp -o main && ./main