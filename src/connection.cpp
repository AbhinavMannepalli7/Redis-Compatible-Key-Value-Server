#include "connection.hpp"

#include <cerrno>
#include <sys/socket.h>
#include <unistd.h>

Connection::Connection(int fd) {
    fd_ = fd;
}

Connection::~Connection() {
    close(fd_);
}

bool Connection::do_read() {
    char buf[4096];

    while (true) {
        ssize_t count = recv(fd_, buf, sizeof(buf), 0);

        if (count > 0) {
            // Add newly received bytes to our input buffer
            in_buffer.append(buf, count);

            // Process every complete message currently in the buffer
            while (true) {
                size_t pos = in_buffer.find('\n');

                if (pos == std::string::npos) {
                    break;
                }

                std::string message = in_buffer.substr(0, pos);

                // Remove message + '\n' from input buffer
                in_buffer.erase(0, pos + 1);

                // Echo the message
                out_buffer += message + '\n';
            }
        }
        else if (count == 0) {
            // Client closed the connection
            return false;
        }
        else {
            // recv() failed
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No more data available right now
                return true;
            }

            // Real error
            return false;
        }
    }
}

bool Connection::do_write() {
    while (!out_buffer.empty()) {
        ssize_t count = send(
            fd_,
            out_buffer.data(),
            out_buffer.size(),
            0
        );

        if (count > 0) {
            out_buffer.erase(0, count);
        }
        else if (count == -1 &&
                 (errno == EAGAIN || errno == EWOULDBLOCK)) {
            // Socket cannot accept more data right now.
            // Keep the remaining data in out_buffer.
            return true;
        }
        else {
            // Real send error
            return false;
        }
    }
    return true;
}

bool Connection::has_data_to_write() const {
    return !out_buffer.empty();
}