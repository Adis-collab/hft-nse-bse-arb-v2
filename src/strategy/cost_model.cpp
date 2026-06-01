#include "strategy/cost_model.hpp"
#include "common/config.hpp"

namespace hft {

CostModel CostModel::load(const std::string& path) {
    auto cfg = SimpleConfig::load(path);
    CostModel c;
    c.fees_paise_per_share = cfg.get_i64("fees_paise_per_share", 0);
    c.taxes_paise_per_share = cfg.get_i64("taxes_paise_per_share", 0);
    c.slippage_buffer_paise = cfg.get_i64("slippage_buffer_paise", 0);
    c.latency_buffer_paise = cfg.get_i64("latency_buffer_paise", 0);
    c.hedge_risk_buffer_paise = cfg.get_i64("hedge_risk_buffer_paise", 0);
    c.min_net_edge_paise = cfg.get_i64("min_net_edge_paise", 0);
    return c;
}

bool CostModel::validate(std::string* reason) const {
    if (fees_paise_per_share <= 0 || taxes_paise_per_share <= 0) {
        if (reason) *reason = "fees/taxes must be positive placeholders or verified values";
        return false;
    }
    if (min_net_edge_paise < 0) {
        if (reason) *reason = "min_net_edge_paise cannot be negative";
        return false;
    }
    if (reason) *reason = "OK";
    return true;
}

int64_t CostModel::round_trip_paise() const {
    return 2 * (fees_paise_per_share + taxes_paise_per_share);
}

int64_t CostModel::buffers_paise() const {
    return slippage_buffer_paise + latency_buffer_paise + hedge_risk_buffer_paise;
}

int64_t CostModel::all_in_cost_paise() const {
    return round_trip_paise() + buffers_paise();
}

} // namespace hft
