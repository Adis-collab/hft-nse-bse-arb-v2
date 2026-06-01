#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include "common/types.hpp"

namespace hft {

struct FeedState {
    uint64_t last_event_ns{};
    bool healthy{true};
    std::string reason;
};

class FeedHealth {
public:
    void on_event(Venue venue, uint32_t instrument_id, uint64_t ts_ingest_ns) {
        auto& s = states_[key(venue, instrument_id)];
        s.last_event_ns = ts_ingest_ns;
        if (s.reason.empty()) s.healthy = true;
    }

    void mark_unhealthy(Venue venue, uint32_t instrument_id, std::string reason) {
        auto& s = states_[key(venue, instrument_id)];
        s.healthy = false;
        s.reason = std::move(reason);
    }

    bool is_healthy(Venue venue, uint32_t instrument_id) const {
        auto it = states_.find(key(venue, instrument_id));
        return it != states_.end() && it->second.healthy;
    }

    uint32_t book_age_us(Venue venue, uint32_t instrument_id, uint64_t now_ns) const {
        auto it = states_.find(key(venue, instrument_id));
        if (it == states_.end() || it->second.last_event_ns == 0) return UINT32_MAX;
        if (now_ns < it->second.last_event_ns) return 0;
        return static_cast<uint32_t>((now_ns - it->second.last_event_ns) / 1000ULL);
    }

    std::string reason(Venue venue, uint32_t instrument_id) const {
        auto it = states_.find(key(venue, instrument_id));
        return it == states_.end() ? "NO_STATE" : it->second.reason;
    }

private:
    static uint64_t key(Venue v, uint32_t instrument_id) {
        return (static_cast<uint64_t>(static_cast<int>(v)) << 32) | instrument_id;
    }

    std::unordered_map<uint64_t, FeedState> states_;
};

} // namespace hft
