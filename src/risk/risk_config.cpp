#include "risk/risk_config.hpp"
#include "common/config.hpp"

namespace hft {

RiskConfig RiskConfig::load(const std::string& path) {
    auto cfg = SimpleConfig::load(path);
    RiskConfig r;
    r.max_order_qty = cfg.get_i64("max_order_qty", r.max_order_qty);
    r.max_position_qty = cfg.get_i64("max_position_qty", r.max_position_qty);
    r.max_notional_paise = cfg.get_i64("max_notional_paise", r.max_notional_paise);
    r.max_book_age_us = static_cast<uint32_t>(cfg.get_i64("max_book_age_us", r.max_book_age_us));
    r.max_orders_per_sec = cfg.get_int("max_orders_per_sec", r.max_orders_per_sec);
    r.max_hedge_attempts = cfg.get_int("max_hedge_attempts", r.max_hedge_attempts);
    r.max_hedge_delay_us = static_cast<uint64_t>(cfg.get_i64("max_hedge_delay_us", r.max_hedge_delay_us));
    r.max_hedge_loss_paise = cfg.get_i64("max_hedge_loss_paise", r.max_hedge_loss_paise);
    r.price_band_paise = cfg.get_i64("price_band_paise", r.price_band_paise);
    r.daily_loss_limit_paise = cfg.get_i64("daily_loss_limit_paise", r.daily_loss_limit_paise);
    r.force_flatten_on_max_loss = cfg.get_bool("force_flatten_on_max_loss", r.force_flatten_on_max_loss);
    return r;
}

} // namespace hft
