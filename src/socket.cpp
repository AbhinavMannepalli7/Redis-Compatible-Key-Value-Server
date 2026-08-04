#include "socket.hpp"

#include <stdexcept>
#include <unistd.h>

#include <sys/socket.h>

Socket::Socket(int domain, int type, int protocol)
{
    fd = socket(domain, type, protocol);

    if (fd == -1)
        throw std::runtime_error("socket() failed");
}

Socket::~Socket()
{
    if (fd != -1)
        close(fd);
}

void Socket::bindSocket(const sockaddr_in& addr)
{
    if (::bind(fd, (sockaddr*)&addr, sizeof(addr)) == -1)
        throw std::runtime_error("bind() failed");
}

void Socket::startListening(int backlog)
{
    if (::listen(fd, backlog) == -1)
        throw std::runtime_error("listen() failed");
}

int Socket::acceptConnection()
{
    int newfd = ::accept(fd, nullptr, nullptr);

    if (newfd == -1)
        throw std::runtime_error("accept() failed");

    return newfd;
}

void Socket::sendData(int clientFd, const std::string& data)
{
    if (::send(clientFd, data.c_str(), data.size(), 0) == -1)
        throw std::runtime_error("send() failed");
}