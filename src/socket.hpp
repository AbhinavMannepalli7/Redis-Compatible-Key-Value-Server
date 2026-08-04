#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <string>
#include <netinet/in.h>

class Socket {
public:
    Socket(int domain, int type, int protocol);
    ~Socket();

    void bindSocket(const sockaddr_in& addr);
    void startListening(int backlog);
    int acceptConnection();
    void sendData(int clientFd, const std::string& data);

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

private:
    int fd = -1;
};

#endif