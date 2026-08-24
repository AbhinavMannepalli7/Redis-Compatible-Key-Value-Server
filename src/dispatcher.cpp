#include "dispatcher.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

static std::string handle_get(const Command& cmd, Store& store) {
    auto val = store.get(cmd.args[0]);
    if (!val) {
        return "$-1\r\n"; 
    }
    const std::string& s = val.value();
    return "$" + std::to_string(s.size()) + "\r\n" + s + "\r\n";
}

static std::string handle_set(const Command& cmd, Store& store) {
    std::string key = cmd.args[0];
    std::string value = cmd.args[1];
    store.set(key, value);
    return "+OK\r\n";
}

static std::string handle_del(const Command& cmd, Store& store) {
    if (cmd.args.empty()) {
        return "-ERR wrong number of arguments for 'del' command\r\n";
    }
    int count = 0;
    for (const std::string& key : cmd.args) {
        if (store.del(key)) {
            count++;
        }
    }
    return ":" + std::to_string(count) + "\r\n";
}

static std::string handle_ping(const Command& cmd) {
    if (cmd.args.size() > 1) {
        return "-ERR wrong number of arguments for 'ping' command\r\n";
    }
    if (cmd.args.size() == 1) {
        const std::string& word = cmd.args[0];
        return "$" + std::to_string(word.size()) + "\r\n" + word + "\r\n";
    }
    return "+PONG\r\n";
}

static std::string handle_exists(const Command& cmd, Store& store) {
    int count = 0;
    for (std::string key : cmd.args) {
        if (store.exists(key)) count++;
    }
    return ":" + std::to_string(count) + "\r\n";
}

static std::string handle_expire(const Command& cmd, Store& store) {
    std::string key = cmd.args[0];
    std::string time = cmd.args[1];
    if (store.expire(key, time)) {
        return "1\r\n";
    }
    return "0\r\n";
}

static std::string handle_ttl(const Command& cmd, Store& store) {
    std::string key = cmd.args[0];
    int64_t res = store.ttl(key);
    return std::to_string(res) + "\r\n";
}

static const std::unordered_map<std::string, std::function<std::string(const Command&, Store&)>> handlers = {
    {"GET", handle_get},
    {"SET", handle_set},
    {"DEL", handle_del},
    {"EXISTS", handle_exists},
    {"EXPIRE", handle_expire},
    {"TTL", handle_ttl},
};

std::string dispatch(const Command& cmd, Store& store) {
    if (cmd.verb == "PING") {
        return handle_ping(cmd);
    }
    auto it = handlers.find(cmd.verb);
    if (it == handlers.end()) return "-ERR unknown command\r\n";
    return it->second(cmd, store);
}