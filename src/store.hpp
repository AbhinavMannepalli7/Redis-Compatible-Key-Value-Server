#pragma once
#include <unordered_map>
#include <string>
#include <optional>
#include <queue>
#include <cstdint>
#include <mutex>

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
    std::optional<int64_t> incr_decr(const std::string& key, const int64_t& delta);
    bool hasExpiry(const std::string& key);
    void handle_expirations();
    std::optional<uint64_t> next_expiry() const;

private:
    // "_locked" versions assume mu_ is already held by the caller.
    // Public methods above take the lock once, then call these internally,
    // so no method ever tries to lock mu_ a second time on the same thread.
    std::optional<std::string> get_locked(const std::string& key);
    void set_locked(const std::string& key, const std::string& value);
    bool del_locked(const std::string& key);
    bool exists_locked(const std::string& key);
    bool hasExpiry_locked(const std::string& key);

    std::unordered_map<std::string, std::string> data_;
    std::unordered_map<std::string, uint64_t> ttl_map;
    std::unordered_map<std::string, uint64_t> keyVersion;
    std::priority_queue<ExpiryEntry, std::vector<ExpiryEntry>, ExpiryComparator> min_heap;

    mutable std::mutex mu_;
};