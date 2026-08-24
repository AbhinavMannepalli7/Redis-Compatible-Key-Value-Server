#include "store.hpp"
#include <chrono>

bool Store::exists(const std::string& key) {
    auto it = data_.find(key);
    return it != data_.end();
}

bool Store::hasExpiry(const std::string& key) {
    auto it = ttl_map.find(key);
    return it != ttl_map.end();
}

std::optional<std::string> Store::get(const std::string& key) {
    if (!exists(key)) return std::nullopt;
    return data_[key];
}

void Store::set(const std::string& key, const std::string& value) {
    data_[key] = value;
    ttl_map.erase(key);
    keyVersion[key]++;
}

bool Store::del(const std::string& key) {
    if (!exists(key)) return false;

    data_.erase(key);
    ttl_map.erase(key);
    keyVersion[key]++;

    return true;
}

bool Store::expire(const std::string& key, const std::string& time) {
    if (!exists(key)) return false;

    uint64_t expiry_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count() + std::stoull(time) * 1000;

    ttl_map[key] = expiry_time;
    ExpiryEntry cur = {expiry_time, key, ++keyVersion[key]};
    min_heap.push(cur);

    return true;
}

int64_t Store::ttl(const std::string& key) {
    if (!exists(key)) return -2;

    if (!hasExpiry(key)) return -1;

    uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();

    if (ttl_map[key] <= now)
        return 0;

    return static_cast<int64_t>((ttl_map[key] - now) / 1000);
}

void Store::handle_expirations() {
    uint64_t now =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();

    while (!min_heap.empty() && min_heap.top().expiry_time <= now) {
        ExpiryEntry cur = min_heap.top();
        min_heap.pop();

        if (cur.version != keyVersion[cur.key])
            continue;

        del(cur.key);
    }
}

std::optional<uint64_t> Store::next_expiry() const {
    if (min_heap.empty())
        return std::nullopt;

    return min_heap.top().expiry_time;
}