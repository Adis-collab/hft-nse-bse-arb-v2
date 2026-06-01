#pragma once

#include <cstdint>
#include <unordered_map>
#include "common/types.hpp"

namespace hft {

struct Position {
    int64_t net_qty{};
    int64_t cash_paise{}; // buys reduce cash, sells increase cash
};

class PositionBook {
public:
    void apply_fill(uint32_t instrument_id, Side side, int64_t qty, int64_t price_paise);
    int64_t net_qty(uint32_t instrument_id) const;
    int64_t cash_paise(uint32_t instrument_id) const;
    Position position(uint32_t instrument_id) const;
    void set_position(uint32_t instrument_id, int64_t qty, int64_t cash_paise = 0);
private:
    std::unordered_map<uint32_t, Position> positions_;
};

} // namespace hft
