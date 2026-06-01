#pragma once

#include <cstdint>
#include <unordered_map>

namespace hft {

class RateLimiter {
public:
    explicit RateLimiter(int max_per_sec = 50) : max_per_sec_(max_per_sec) {}

    bool allow(uint32_t instrument_id, uint64_t now_ns) {
        uint64_t sec = now_ns / 1000000000ULL;
        auto& b = buckets_[instrument_id];
        if (b.second != sec) {
            b.first = 0;
            b.second = sec;
        }
        if (b.first >= max_per_sec_) return false;
        ++b.first;
        return true;
    }

    void set_limit(int max_per_sec) { max_per_sec_ = max_per_sec; }

private:
    int max_per_sec_;
    // value: count, second
    std::unordered_map<uint32_t, std::pair<int, uint64_t>> buckets_;
};

} // namespace hft
