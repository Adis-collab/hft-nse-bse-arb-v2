#include "hedge/pair_trade.hpp"

namespace hft {

std::string to_string(PairState s) {
    switch (s) {
        case PairState::Planned: return "PLANNED";
        case PairState::SentLegA: return "SENT_LEG_A";
        case PairState::SentLegB: return "SENT_LEG_B";
        case PairState::PartialA: return "PARTIAL_A";
        case PairState::PartialB: return "PARTIAL_B";
        case PairState::Hedging: return "HEDGING";
        case PairState::Flat: return "FLAT";
        case PairState::ErrorRecovery: return "ERROR_RECOVERY";
        case PairState::KillSwitch: return "KILL_SWITCH";
        default: return "UNKNOWN";
    }
}

PairTrade PairTrade::from_opportunity(const ArbOpportunity& o) {
    PairTrade p;
    p.parent_id = o.parent_arb_id;
    p.instrument_id = o.instrument_id;
    p.intended_buy_venue = o.buy_venue;
    p.intended_sell_venue = o.sell_venue;
    p.intended_qty = o.qty;
    p.ts_created_ns = o.ts_decision_ns;
    p.ts_last_update_ns = o.ts_decision_ns;
    p.state = PairState::Planned;
    return p;
}

void PairTrade::apply_fill(const Fill& f) {
    ts_last_update_ns = f.ts_fill_ns;
    if (f.side == Side::Buy) {
        bought_qty += f.fill_qty;
        bought_notional_paise += f.fill_qty * f.fill_px_paise;
    } else if (f.side == Side::Sell) {
        sold_qty += f.fill_qty;
        sold_notional_paise += f.fill_qty * f.fill_px_paise;
    }
    if (residual() == 0 && (bought_qty > 0 || sold_qty > 0)) state = PairState::Flat;
    else state = PairState::Hedging;
}

void PairTrade::mark_leg_failed(std::string reason, uint64_t ts_ns) {
    last_error = std::move(reason);
    ts_last_update_ns = ts_ns;
    if (residual() == 0) state = PairState::Flat;
    else state = PairState::Hedging;
}

int64_t PairTrade::residual() const {
    return bought_qty - sold_qty;
}

int64_t PairTrade::avg_buy_px() const {
    return bought_qty == 0 ? 0 : bought_notional_paise / bought_qty;
}

int64_t PairTrade::avg_sell_px() const {
    return sold_qty == 0 ? 0 : sold_notional_paise / sold_qty;
}

} // namespace hft
