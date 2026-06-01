#pragma once

#include <cstdint>
#include <string>

namespace hft {

struct CostModel {
    int64_t fees_paise_per_share{0};
    int64_t taxes_paise_per_share{0};
    int64_t slippage_buffer_paise{0};
    int64_t latency_buffer_paise{0};
    int64_t hedge_risk_buffer_paise{0};
    int64_t min_net_edge_paise{0};

    static CostModel load(const std::string& path);
    bool validate(std::string* reason = nullptr) const;
    int64_t round_trip_paise() const;
    int64_t buffers_paise() const;
    int64_t all_in_cost_paise() const;
};

} // namespace hft
