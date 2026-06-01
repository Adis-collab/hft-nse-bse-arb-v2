#include "risk/position_book.hpp"

namespace hft {

void PositionBook::apply_fill(uint32_t instrument_id, Side side, int64_t qty, int64_t price_paise) {
    auto& p = positions_[instrument_id];
    if (side == Side::Buy) {
        p.net_qty += qty;
        p.cash_paise -= qty * price_paise;
    } else if (side == Side::Sell) {
        p.net_qty -= qty;
        p.cash_paise += qty * price_paise;
    }
}

int64_t PositionBook::net_qty(uint32_t instrument_id) const {
    auto it = positions_.find(instrument_id);
    return it == positions_.end() ? 0 : it->second.net_qty;
}

int64_t PositionBook::cash_paise(uint32_t instrument_id) const {
    auto it = positions_.find(instrument_id);
    return it == positions_.end() ? 0 : it->second.cash_paise;
}

Position PositionBook::position(uint32_t instrument_id) const {
    auto it = positions_.find(instrument_id);
    return it == positions_.end() ? Position{} : it->second;
}

void PositionBook::set_position(uint32_t instrument_id, int64_t qty, int64_t cash_paise) {
    positions_[instrument_id] = Position{qty, cash_paise};
}

} // namespace hft
