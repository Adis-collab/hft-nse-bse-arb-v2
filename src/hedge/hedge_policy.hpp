#pragma once

#include <optional>
#include <string>
#include "book/best_quote.hpp"
#include "hedge/pair_trade.hpp"
#include "oms/order_intent.hpp"
#include "risk/risk_config.hpp"

namespace hft {

struct HedgeDecision {
    bool should_send{false};
    bool kill{false};
    OrderIntent intent;
    std::string reason;
    int64_t estimated_loss_paise{};
};

class HedgePolicy {
public:
    explicit HedgePolicy(RiskConfig cfg) : cfg_(cfg) {}
    HedgeDecision next_order(const PairTrade& p, const BestQuote& nse, const BestQuote& bse) const;

private:
    HedgeDecision candidate_for_venue(const PairTrade& p, Venue v, const BestQuote& q) const;
    RiskConfig cfg_;
};

} // namespace hft
