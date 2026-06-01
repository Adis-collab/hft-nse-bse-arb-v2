#include "hedge/hedge_policy.hpp"
#include <algorithm>
#include <cstdlib>

namespace hft {

HedgeDecision HedgePolicy::next_order(const PairTrade& p, const BestQuote& nse, const BestQuote& bse) const {
    if (p.residual() == 0) return HedgeDecision{false, false, {}, "ALREADY_FLAT", 0};
    if (p.hedge_attempts >= cfg_.max_hedge_attempts) return HedgeDecision{false, true, {}, "MAX_HEDGE_ATTEMPTS", 0};

    Venue preferred = p.residual() > 0 ? p.intended_sell_venue : p.intended_buy_venue;
    Venue fallback = other_venue(preferred);

    const BestQuote& q1 = preferred == Venue::NSE ? nse : bse;
    auto d1 = candidate_for_venue(p, preferred, q1);
    if (d1.should_send || d1.kill) return d1;

    const BestQuote& q2 = fallback == Venue::NSE ? nse : bse;
    auto d2 = candidate_for_venue(p, fallback, q2);
    if (d2.should_send || d2.kill) return d2;

    return HedgeDecision{false, true, {}, "NO_SAFE_HEDGE_QUOTE", 0};
}

HedgeDecision HedgePolicy::candidate_for_venue(const PairTrade& p, Venue v, const BestQuote& q) const {
    if (!q.valid()) return HedgeDecision{false, false, {}, "INVALID_HEDGE_QUOTE", 0};
    if (q.book_age_us > cfg_.max_book_age_us) return HedgeDecision{false, false, {}, "STALE_HEDGE_QUOTE", 0};

    int64_t r = p.residual();
    OrderIntent intent;
    intent.parent_arb_id = p.parent_id;
    intent.instrument_id = p.instrument_id;
    intent.venue = v;
    intent.order_type = OrderType::Limit;
    intent.tif = TimeInForce::IOC;
    intent.role = OrderRole::Hedge;

    int64_t loss = 0;
    if (r > 0) {
        // Long residual: sell to flatten.
        intent.side = Side::Sell;
        intent.price_paise = q.bid_px_paise;
        intent.qty = std::min({r, q.bid_qty, cfg_.max_order_qty});
        int64_t ref = p.avg_buy_px();
        if (ref > intent.price_paise) loss = (ref - intent.price_paise) * intent.qty;
    } else {
        // Short residual: buy to flatten.
        int64_t need = -r;
        intent.side = Side::Buy;
        intent.price_paise = q.ask_px_paise;
        intent.qty = std::min({need, q.ask_qty, cfg_.max_order_qty});
        int64_t ref = p.avg_sell_px();
        if (ref > 0 && intent.price_paise > ref) loss = (intent.price_paise - ref) * intent.qty;
    }

    if (intent.qty <= 0) return HedgeDecision{false, false, {}, "NO_HEDGE_DEPTH", 0};
    if (loss > cfg_.max_hedge_loss_paise && !cfg_.force_flatten_on_max_loss) {
        return HedgeDecision{false, true, {}, "MAX_HEDGE_LOSS", loss};
    }
    return HedgeDecision{true, false, intent, "HEDGE_ORDER", loss};
}

} // namespace hft
