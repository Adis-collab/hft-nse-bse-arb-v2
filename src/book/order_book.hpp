#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include "book/best_quote.hpp"
#include "md/book_event.hpp"

namespace hft {

struct RestingOrder {
    Side side{Side::Unknown};
    int64_t price_paise{};
    int64_t qty{};
};

struct LevelFill {
    int64_t price_paise{};
    int64_t qty{};
};

class OrderBook {
public:
    OrderBook() = default;
    OrderBook(Venue venue, uint32_t instrument_id) : venue_(venue), instrument_id_(instrument_id) {}

    void apply(const BookEvent& ev);
    void add(uint64_t id, Side side, int64_t px, int64_t qty);
    void modify(uint64_t id, Side side, int64_t new_px, int64_t new_qty);
    void cancel(uint64_t id);
    void trade(uint64_t resting_id, Side side, int64_t px, int64_t qty);
    std::vector<LevelFill> match_ioc(Side aggressor_side, int64_t limit_px, int64_t qty);

    BestQuote best_quote(uint64_t now_ns) const;
    bool invariant_ok(std::string* reason = nullptr) const;
    bool crossed() const;
    int64_t depth_at(Side side, int64_t price_paise) const;
    int64_t total_depth(Side side) const;

private:
    using BidMap = std::map<int64_t, int64_t, std::greater<int64_t>>;
    using AskMap = std::map<int64_t, int64_t>;

    void remove_from_level(Side side, int64_t px, int64_t qty);
    void consume_orders_at_price(Side resting_side, int64_t px, int64_t qty_to_consume);
    void clear();

    Venue venue_{Venue::Unknown};
    uint32_t instrument_id_{};
    uint64_t last_update_ns_{};
    BidMap bids_;
    AskMap asks_;
    std::unordered_map<uint64_t, RestingOrder> orders_;
};

} // namespace hft
