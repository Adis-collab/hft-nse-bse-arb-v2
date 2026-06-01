#pragma once

#include <cstdint>
#include <string>
#include "common/types.hpp"

namespace hft {

struct OrderIntent {
    std::string parent_arb_id;
    uint32_t instrument_id{};
    Venue venue{Venue::Unknown};
    Side side{Side::Unknown};
    OrderType order_type{OrderType::Limit};
    TimeInForce tif{TimeInForce::IOC};
    OrderRole role{OrderRole::Unknown};
    int64_t price_paise{};
    int64_t qty{};
};

} // namespace hft
