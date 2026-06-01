#include "strategy/arbitrage_strategy.hpp"
#include <algorithm>

namespace hft {

namespace {
StrategyDecision make_no_trade(std::string reason, int64_t gross = 0, int64_t net = 0) {
    StrategyDecision d;
    d.should_trade = false;
    d.reason = std::move(reason);
    d.best_gross_spread_paise = gross;
    d.best_net_edge_paise = net;
    return d;
}
}

StrategyDecision ArbitrageStrategy::evaluate(const BestQuote& nse, const BestQuote& bse, const CostModel& costs, uint64_t now_ns) const {
    if (!nse.valid() || !bse.valid()) return make_no_trade("INVALID_QUOTE");
    if (nse.instrument_id != bse.instrument_id) return make_no_trade("INSTRUMENT_MISMATCH");
    if (nse.book_age_us > cfg_.max_book_age_us || bse.book_age_us > cfg_.max_book_age_us) return make_no_trade("STALE_BOOK");

    auto try_direction = [&](Venue buy_venue, Venue sell_venue,
                             int64_t buy_px, int64_t buy_qty,
                             int64_t sell_px, int64_t sell_qty) {
        ArbOpportunity o;
        o.ts_decision_ns = now_ns;
        o.instrument_id = nse.instrument_id;
        o.buy_venue = buy_venue;
        o.sell_venue = sell_venue;
        o.buy_price_paise = buy_px;
        o.sell_price_paise = sell_px;
        o.gross_spread_paise = sell_px - buy_px;
        o.net_edge_paise = o.gross_spread_paise - costs.all_in_cost_paise();
        o.qty = std::min({buy_qty, sell_qty, cfg_.max_qty});
        o.reason = "NET_EDGE_POSITIVE";
        return o;
    };

    ArbOpportunity a = try_direction(Venue::BSE, Venue::NSE, bse.ask_px_paise, bse.ask_qty, nse.bid_px_paise, nse.bid_qty);
    ArbOpportunity b = try_direction(Venue::NSE, Venue::BSE, nse.ask_px_paise, nse.ask_qty, bse.bid_px_paise, bse.bid_qty);
    ArbOpportunity best = (a.net_edge_paise >= b.net_edge_paise) ? a : b;

    if (best.qty < cfg_.min_visible_qty) return make_no_trade("INSUFFICIENT_DEPTH", best.gross_spread_paise, best.net_edge_paise);
    int64_t threshold = std::max(cfg_.min_net_edge_paise, costs.min_net_edge_paise);
    if (best.net_edge_paise < threshold) return make_no_trade("EDGE_TOO_SMALL", best.gross_spread_paise, best.net_edge_paise);

    StrategyDecision d;
    d.should_trade = true;
    d.opportunity = best;
    d.reason = best.reason;
    d.best_gross_spread_paise = best.gross_spread_paise;
    d.best_net_edge_paise = best.net_edge_paise;
    return d;
}

} // namespace hft
