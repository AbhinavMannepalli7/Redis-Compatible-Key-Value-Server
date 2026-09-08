#include "socket.hpp"

#include <stdexcept>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>

Socket::Socket(int domain, int type, int protocol) {
    fd_ = socket(domain, type, protocol);

    if (fd_ == -1) throw std::runtime_error("socket() failed");
}

Socket::~Socket() {
    if (fd_ != -1) close(fd_);
}

void Socket::bindSocket(const sockaddr_in& addr) {
    if (bind(fd_, (sockaddr*)&addr, sizeof(addr)) == -1) {
        throw std::runtime_error("bind() failed");
    }
}

void Socket::startListening(int backlog) {
    if (listen(fd_, backlog) == -1) {
        throw std::runtime_error("listen() failed");
    }
}

int Socket::acceptConnection() {
    int newfd = accept(fd_, nullptr, nullptr);

    if (newfd == -1) {
        throw std::runtime_error("accept() failed");
    }
    return newfd;
}

void Socket::sendData(int clientFd, const std::string& data) {
    if (send(clientFd, data.c_str(), data.size(), 0) == -1) {
        throw std::runtime_error("send() failed");
    }
}

int Socket::set_nonblocking(int target_fd) {
    int flags = fcntl(target_fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(target_fd, F_SETFL, flags | O_NONBLOCK);
}

int Socket::fd() const { 
    return fd_;
}