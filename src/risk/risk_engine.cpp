#include "risk/risk_engine.hpp"
#include <cstdlib>

namespace hft {

RiskDecision RiskEngine::check_order(const OrderIntent& o, const BestQuote& q, uint64_t now_ns) {
    if (!kill_.enabled()) return RiskDecision::block("KILL_SWITCH", kill_.reason());
    if (!q.valid()) return RiskDecision::block("INVALID_QUOTE", "quote missing or one-sided");
    if (q.book_age_us > cfg_.max_book_age_us) return RiskDecision::block("STALE_BOOK", "quote age too high");
    if (o.qty <= 0 || o.qty > cfg_.max_order_qty) return RiskDecision::block("MAX_QTY", "order quantity outside limits");
    if (o.price_paise <= 0) return RiskDecision::block("BAD_PRICE", "price must be positive");
    if (o.qty * o.price_paise > cfg_.max_notional_paise) return RiskDecision::block("MAX_NOTIONAL", "order notional too high");

    int64_t signed_effect = (o.side == Side::Buy) ? o.qty : -o.qty;
    if (std::llabs(positions_.net_qty(o.instrument_id) + signed_effect) > cfg_.max_position_qty) {
        return RiskDecision::block("MAX_POSITION", "position cap exceeded");
    }

    if (o.side == Side::Buy && q.ask_px_paise > 0 && o.price_paise > q.ask_px_paise + cfg_.price_band_paise) {
        return RiskDecision::block("PRICE_BAND", "buy price too far from ask");
    }
    if (o.side == Side::Sell && q.bid_px_paise > 0 && o.price_paise < q.bid_px_paise - cfg_.price_band_paise) {
        return RiskDecision::block("PRICE_BAND", "sell price too far from bid");
    }

    if (!limiter_.allow(o.instrument_id, now_ns)) return RiskDecision::block("ORDER_RATE", "order rate exceeded");
    return RiskDecision::approve();
}

} // namespace hft
