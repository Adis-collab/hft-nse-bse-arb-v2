#pragma once

#include <cstdint>
#include <string>
#include "common/types.hpp"

namespace hft {

struct RawPacket {
    uint64_t ts_ingest_ns{};
    Venue venue{Venue::Unknown};
    uint32_t channel_id{};
    uint64_t sequence_no{};
    std::string msg_type;
    std::string src_ip;
    std::string payload_bytes;
};

} // namespace hft
