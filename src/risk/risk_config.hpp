#pragma once

#include <cstdint>
#include <string>

namespace hft {

struct RiskConfig {
    int64_t max_order_qty{100};
    int64_t max_position_qty{200};
    int64_t max_notional_paise{20000000};
    uint32_t max_book_age_us{250};
    int max_orders_per_sec{50};
    int max_hedge_attempts{3};
    uint64_t max_hedge_delay_us{2000};
    int64_t max_hedge_loss_paise{2000};
    int64_t price_band_paise{100};
    int64_t daily_loss_limit_paise{100000};
    bool force_flatten_on_max_loss{false};

    static RiskConfig load(const std::string& path);
};

} // namespace hft
