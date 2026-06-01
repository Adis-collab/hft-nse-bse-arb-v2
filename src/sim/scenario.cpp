#include "sim/scenario.hpp"

namespace hft {

ScenarioConfig ScenarioConfig::from_name(const std::string& name) {
    ScenarioConfig s;
    s.name = name;
    s.description = name;
    if (name == "bse_buy_fills_nse_sell_reject") {
        s.reject_roles_once.insert(OrderRole::AlphaSell);
    } else if (name == "nse_sell_fills_bse_buy_reject") {
        s.reject_roles_once.insert(OrderRole::AlphaBuy);
        s.send_sell_first = true;
    } else if (name == "both_legs_partial") {
        s.max_fill_qty_by_role[OrderRole::AlphaBuy] = 60;
        s.max_fill_qty_by_role[OrderRole::AlphaSell] = 20;
    } else if (name == "buy_partial_sell_zero") {
        s.max_fill_qty_by_role[OrderRole::AlphaBuy] = 60;
        s.reject_roles_once.insert(OrderRole::AlphaSell);
    } else if (name == "gateway_disconnect_nse") {
        s.disconnect_nse = true;
    } else if (name == "delayed_ack") {
        s.delayed_ack = true;
    } else if (name == "manual_kill") {
        s.manual_kill = true;
    }
    return s;
}

bool ScenarioConfig::should_reject(OrderRole role) {
    auto it = reject_roles_once.find(role);
    if (it == reject_roles_once.end()) return false;
    reject_roles_once.erase(it);
    return true;
}

int64_t ScenarioConfig::cap_fill_qty(OrderRole role, int64_t requested) const {
    auto it = max_fill_qty_by_role.find(role);
    if (it == max_fill_qty_by_role.end()) return requested;
    return std::min(requested, it->second);
}

bool ScenarioConfig::disconnected(Venue v) const {
    return (v == Venue::NSE && disconnect_nse) || (v == Venue::BSE && disconnect_bse);
}

} // namespace hft
