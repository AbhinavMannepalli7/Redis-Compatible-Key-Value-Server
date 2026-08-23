#pragma once
#include <unordered_map>
#include <string>
#include <optional>

class Store {
public:
    std::optional<std::string> get(const std::string& key);
    void set(const std::string& key, const std::string& value);
    bool del(const std::string& key);
    bool exists(const std::string& key);
private:
    std::unordered_map<std::string, std::string> data_;
};