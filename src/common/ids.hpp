#pragma once

#include <atomic>
#include <sstream>
#include <string>
#include "common/types.hpp"

namespace hft {

class IdGenerator {
public:
    explicit IdGenerator(std::string prefix = "HFT") : prefix_(std::move(prefix)) {}

    std::string next_parent(uint32_t instrument_id) {
        uint64_t n = parent_seq_.fetch_add(1, std::memory_order_relaxed) + 1;
        std::ostringstream oss;
        oss << prefix_ << "-ARB-" << instrument_id << "-" << n;
        return oss.str();
    }

    std::string next_order(const std::string& parent_id, Venue venue, Side side, OrderRole role) {
        uint64_t n = order_seq_.fetch_add(1, std::memory_order_relaxed) + 1;
        std::ostringstream oss;
        oss << parent_id << "-" << to_string(venue) << "-" << to_string(side) << "-" << to_string(role) << "-" << n;
        return oss.str();
    }

private:
    std::string prefix_;
    std::atomic<uint64_t> parent_seq_{0};
    std::atomic<uint64_t> order_seq_{0};
};

} // namespace hft
