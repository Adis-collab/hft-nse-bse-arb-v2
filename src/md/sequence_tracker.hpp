#pragma once

#include <cstdint>
#include <unordered_map>
#include "common/types.hpp"

namespace hft {

enum class SequenceStatus { Ok, Gap, Duplicate, Reset };

class SequenceTracker {
public:
    SequenceStatus on_packet(Venue venue, uint32_t channel, uint64_t seq) {
        uint64_t key = (static_cast<uint64_t>(static_cast<int>(venue)) << 32) | channel;
        auto& last = last_seq_[key];
        if (seq == 0) return SequenceStatus::Reset;
        if (last == 0 || seq == last + 1) {
            last = seq;
            return SequenceStatus::Ok;
        }
        if (seq <= last) {
            return SequenceStatus::Duplicate;
        }
        last = seq;
        return SequenceStatus::Gap;
    }

    void reset() { last_seq_.clear(); }

private:
    std::unordered_map<uint64_t, uint64_t> last_seq_;
};

inline const char* to_cstr(SequenceStatus s) {
    switch (s) {
        case SequenceStatus::Ok: return "OK";
        case SequenceStatus::Gap: return "GAP";
        case SequenceStatus::Duplicate: return "DUPLICATE";
        case SequenceStatus::Reset: return "RESET";
        default: return "UNKNOWN";
    }
}

} // namespace hft
