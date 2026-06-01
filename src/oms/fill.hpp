#pragma once

#include <cstdint>
#include <string>
#include "common/types.hpp"

namespace hft {

struct Fill {
    std::string parent_arb_id;
    std::string client_order_id;
    uint32_t instrument_id{};
    Venue venue{Venue::Unknown};
    Side side{Side::Unknown};
    int64_t fill_px_paise{};
    int64_t fill_qty{};
    uint64_t ts_fill_ns{};
};

} // namespace hft
