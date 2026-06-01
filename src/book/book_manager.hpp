#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include "book/order_book.hpp"

namespace hft {

class BookManager {
public:
    BestQuote on_event(const BookEvent& ev);
    std::optional<BestQuote> quote(Venue venue, uint32_t instrument_id, uint64_t now_ns) const;
    OrderBook* book(Venue venue, uint32_t instrument_id);
    const OrderBook* book(Venue venue, uint32_t instrument_id) const;
    bool invariant_ok(std::string* reason = nullptr) const;

private:
    static std::string key(Venue venue, uint32_t instrument_id);
    std::unordered_map<std::string, OrderBook> books_;
};

} // namespace hft
