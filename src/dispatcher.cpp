#include "dispatcher.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

static std::string handle_get(const Command& cmd) {
    return "$-1\r\n";
}

static std::string handle_set(const Command& cmd) {
    return "+OK\r\n";
}

static std::string handle_del(const Command& cmd) {
    return ":0\r\n";
}

static std::string handle_ping(const Command& cmd) {
    return "+PONG\r\n";
}

static const std::unordered_map<std::string, std::function<std::string(const Command&)>> handlers = {
    {"GET",  handle_get},
    {"SET",  handle_set},
    {"DEL",  handle_del},
    {"PING", handle_ping},
};

std::string dispatch(const Command& cmd) {
    auto it = handlers.find(cmd.verb);
    if (it == handlers.end()) return "-ERR unknown command\r\n";
    return it->second(cmd);
}