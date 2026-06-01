#pragma once

#include <cstdint>
#include <string>
#include "common/types.hpp"

namespace hft {

struct ArbOpportunity {
    std::string parent_arb_id;
    uint64_t ts_decision_ns{};
    uint32_t instrument_id{};
    Venue buy_venue{Venue::Unknown};
    Venue sell_venue{Venue::Unknown};
    int64_t buy_price_paise{};
    int64_t sell_price_paise{};
    int64_t gross_spread_paise{};
    int64_t net_edge_paise{};
    int64_t qty{};
    std::string reason;
};

struct StrategyDecision {
    bool should_trade{false};
    ArbOpportunity opportunity;
    std::string reason;
    int64_t best_gross_spread_paise{};
    int64_t best_net_edge_paise{};
};

} // namespace hft
