#pragma once

#include "book/best_quote.hpp"
#include "strategy/cost_model.hpp"
#include "strategy/opportunity.hpp"

namespace hft {

struct StrategyConfig {
    int64_t min_net_edge_paise{5};
    int64_t max_qty{100};
    uint32_t max_book_age_us{250};
    int64_t min_visible_qty{1};
};

class ArbitrageStrategy {
public:
    explicit ArbitrageStrategy(StrategyConfig cfg = {}) : cfg_(cfg) {}
    StrategyDecision evaluate(const BestQuote& nse, const BestQuote& bse, const CostModel& costs, uint64_t now_ns) const;
    const StrategyConfig& config() const { return cfg_; }

private:
    StrategyConfig cfg_;
};

} // namespace hft
