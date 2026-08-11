#include "socket.hpp"

#include <stdexcept>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>

typedef long long ll;
#define PORT 4950
#define BACKLOG 100
#define MAX_EVENTS 64

void handle_new_connections(int listen_fd, int epoll_fd) {
    while (true) {
        int client_fd = accept(listen_fd, nullptr, nullptr);
        if (client_fd == -1) {
            // if the "to be accepted" queue is empty, then we can close loop
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            perror("accept");
            break;
        }
        Socket::set_nonblocking(client_fd);
        // add new client_fd to list of current clients in events list
        struct epoll_event ev{};
        ev.events = EPOLLIN;
        ev.data.fd = client_fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);
    }
}

void handle_client_readable(int client_fd, int epoll_fd, uint32_t events) {
    char buf[4096];
    ssize_t count = recv(client_fd, buf, sizeof(buf), NULL);

    if (count == 0) {
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
        close(client_fd);
        return;
    }
    if (count == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) return; // nothing to read right now
        perror("read");
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
        close(client_fd);
        return;
    }

    send(client_fd, buf, count, NULL);
}
 
int main(void) {
    struct sockaddr_in addrinfo;
    addrinfo.sin_family = AF_INET;
    addrinfo.sin_port = htons(PORT);
    addrinfo.sin_addr.s_addr = htonl(INADDR_ANY);
    
    Socket listener(AF_INET, SOCK_STREAM, 0);
    Socket::set_nonblocking(listener.fd());

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

    while (true) {
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (n == -1) { perror("epoll_wait"); break; }

        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;

            if (fd == listener.fd()) {
                // new connection waiting, accept them
                handle_new_connections(listener.fd(), epoll_fd);
            } else {
                // existing client sent data
                handle_client_readable(fd, epoll_fd, events[i].events);
            }
        }
    }
    return 0;
}
 
// g++-15 -std=c++23 -Wall main.cpp socket.cpp -o main && ./main