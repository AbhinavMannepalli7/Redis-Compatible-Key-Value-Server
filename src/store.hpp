#pragma once
#include <unordered_map>
#include <string>
#include <optional>
#include <queue>
#include <cstdint>

struct ExpiryEntry {
    uint64_t expiry_time;
    std::string key;
    uint64_t version;
};

struct ExpiryComparator {
    bool operator()(const ExpiryEntry& a, const ExpiryEntry& b) const {
        return a.expiry_time > b.expiry_time;
    }
};

class Store {
public:
    std::optional<std::string> get(const std::string& key);
    void set(const std::string& key, const std::string& value);
    bool del(const std::string& key);
    bool exists(const std::string& key);
    bool expire(const std::string& key, const std::string& time);
    int64_t ttl(const std::string& key);
    bool hasExpiry(const std::string& key);
    void handle_expirations();
    std::optional<uint64_t> next_expiry() const;
private:
    std::unordered_map<std::string, std::string> data_;
    std::unordered_map<std::string, uint64_t> ttl_map;
    std::unordered_map<std::string, uint64_t> keyVersion;
    std::priority_queue<ExpiryEntry, std::vector<ExpiryEntry>, ExpiryComparator> min_heap;
};