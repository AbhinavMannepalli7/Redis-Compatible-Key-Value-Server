#include "store.hpp"

std::optional<std::string> Store::get(const std::string& key) {
    auto it = data_.find(key);
    if (it == data_.end()) {
        return std::nullopt;
    }
    return it->second;
}

void Store::set(const std::string& key, const std::string& value) {
    data_[key] = value;
}

bool Store::del(const std::string& key) {
    auto it = data_.find(key);
    if (it != data_.end()) {
        data_.erase(it);
        return true;
    }
    return false;
}

bool Store::exists(const std::string& key) {
    auto it = data_.find(key);
    return it != data_.end();
}