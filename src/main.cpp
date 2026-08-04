#include <iostream>
#include "socket.hpp"

using namespace std;
typedef long long ll;
#define PORT 4950
#define BACKLOG 100
 
int main(void) {
    struct sockaddr_in addrinfo;
    addrinfo.sin_family = AF_INET;
    addrinfo.sin_port = htons(PORT);
    addrinfo.sin_addr.s_addr = INADDR_ANY;

    Socket sock(AF_INET, SOCK_STREAM, 0);
    sock.bindSocket(addrinfo);
    sock.startListening(BACKLOG);
    int newfd = sock.acceptConnection();
    string s = "Hello Mate";
    sock.sendData(newfd, s);
    return 0;
}
 
// g++-15 -std=c++23 -Wall main.cpp socket.cpp -o main && ./main