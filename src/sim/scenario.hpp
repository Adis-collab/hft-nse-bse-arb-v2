#pragma once

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include "common/types.hpp"

namespace hft {

struct ScenarioConfig {
    std::string name{"clean"};
    std::string description;
    bool send_sell_first{false};
    bool delayed_ack{false};
    bool disconnect_nse{false};
    bool disconnect_bse{false};
    bool manual_kill{false};
    std::set<OrderRole> reject_roles_once;
    std::map<OrderRole, int64_t> max_fill_qty_by_role;
    int64_t extra_latency_us{0};

    static ScenarioConfig from_name(const std::string& name);
    bool should_reject(OrderRole role);
    int64_t cap_fill_qty(OrderRole role, int64_t requested) const;
    bool disconnected(Venue v) const;
};

} // namespace hft
