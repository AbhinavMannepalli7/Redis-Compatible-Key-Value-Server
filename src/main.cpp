#include "socket.hpp"
#include "connection.hpp"

#include <stdexcept>
#include <unistd.h>
#include <unordered_map>
#include <memory>
#include <fcntl.h>
#include <sys/epoll.h>

typedef long long ll;
#define PORT 4950
#define BACKLOG 100
#define MAX_EVENTS 64

void handle_new_connections(int listen_fd, int epoll_fd, std::unordered_map<int, std::unique_ptr<Connection>>& connections) {
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
        connections[client_fd] = std::make_unique<Connection>(client_fd);
    }
}
 
int main(void) {
    struct sockaddr_in addrinfo;
    addrinfo.sin_family = AF_INET;
    addrinfo.sin_port = htons(PORT);
    addrinfo.sin_addr.s_addr = htonl(INADDR_ANY);
    
    Socket listener(AF_INET, SOCK_STREAM, 0);
    Socket::set_nonblocking(listener.fd());

    int opt = 1;

    if (setsockopt(listener.fd(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
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

    while (true) {
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (n == -1) { perror("epoll_wait"); break; }

        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;

            if (fd == listener.fd()) {
                // new connection waiting, accept them
                handle_new_connections(listener.fd(), epoll_fd, connections);
            } else {
                // existing client sent data
                //handle_client_readable(fd, epoll_fd, events[i].events);
                if (events[i].events & EPOLLIN) {

                    if (!connections[fd]->do_read()) {
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
                        connections.erase(fd);
                        continue;
                    }
                
                    if (connections[fd]->has_data_to_write()) {
                        struct epoll_event ev{};
                        ev.events = EPOLLIN | EPOLLOUT;
                        ev.data.fd = fd;
                
                        epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev);
                    }
                }
                if (events[i].events & EPOLLOUT) {

                    if (!connections[fd]->do_write()) {
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
                        connections.erase(fd);
                        continue;
                    }
                
                    if (!connections[fd]->has_data_to_write()) {
                        struct epoll_event ev{};
                        ev.events = EPOLLIN;
                        ev.data.fd = fd;
                
                        epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev);
                    }
                }
            }
        }
    }
    return 0;
}
 
// g++-15 -std=c++23 -Wall main.cpp socket.cpp connection.cpp -o main && ./main