#pragma once

#include <cstdint>
#include <string>
#include "common/types.hpp"
#include "oms/fill.hpp"
#include "strategy/opportunity.hpp"

namespace hft {

enum class PairState { Planned, SentLegA, SentLegB, PartialA, PartialB, Hedging, Flat, ErrorRecovery, KillSwitch };

std::string to_string(PairState s);

struct PairTrade {
    std::string parent_id;
    uint32_t instrument_id{};
    Venue intended_buy_venue{Venue::Unknown};
    Venue intended_sell_venue{Venue::Unknown};
    int64_t intended_qty{};
    int64_t bought_qty{};
    int64_t sold_qty{};
    int64_t bought_notional_paise{};
    int64_t sold_notional_paise{};
    int hedge_attempts{};
    uint64_t ts_created_ns{};
    uint64_t ts_last_update_ns{};
    PairState state{PairState::Planned};
    std::string last_error;

    static PairTrade from_opportunity(const ArbOpportunity& o);
    void apply_fill(const Fill& f);
    void mark_leg_failed(std::string reason, uint64_t ts_ns);
    int64_t residual() const;
    int64_t avg_buy_px() const;
    int64_t avg_sell_px() const;
    bool flat() const { return residual() == 0; }
};

} // namespace hft
