#pragma once

#include <cstdint>
#include <string>
#include "common/types.hpp"

namespace hft {

struct BookEvent {
    uint64_t ts_exchange_ns{};
    uint64_t ts_ingest_ns{};
    Venue venue{Venue::Unknown};
    uint32_t instrument_id{};
    uint32_t native_token{};
    EventType event_type{EventType::Unknown};
    uint64_t order_id{};
    Side side{Side::Unknown};
    int64_t price_paise{};
    int64_t qty{};
    std::string native_payload;
};

} // namespace hft
