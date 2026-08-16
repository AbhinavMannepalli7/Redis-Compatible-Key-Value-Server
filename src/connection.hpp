#pragma once

#include <string>

class Connection {
public:
    Connection(int fd);
    ~Connection();

    bool do_read();
    bool do_write();
    bool has_data_to_write() const;
    bool closed_read() const;

private:
    int fd_;

    std::string in_buffer;
    std::string out_buffer;

    size_t bytes_sent_ = 0;
    bool peer_closed_read = false;
};