#pragma once

#include "book/best_quote.hpp"
#include "oms/order_intent.hpp"
#include "risk/kill_switch.hpp"
#include "risk/position_book.hpp"
#include "risk/rate_limiter.hpp"
#include "risk/risk_config.hpp"
#include "risk/risk_event.hpp"

namespace hft {

class RiskEngine {
public:
    RiskEngine(RiskConfig cfg, PositionBook& positions)
        : cfg_(cfg), positions_(positions), limiter_(cfg.max_orders_per_sec) {}

    RiskDecision check_order(const OrderIntent& o, const BestQuote& q, uint64_t now_ns);
    KillSwitch& kill_switch() { return kill_; }
    const RiskConfig& config() const { return cfg_; }
    RateLimiter& limiter() { return limiter_; }

private:
    RiskConfig cfg_;
    PositionBook& positions_;
    RateLimiter limiter_;
    KillSwitch kill_;
};

} // namespace hft
