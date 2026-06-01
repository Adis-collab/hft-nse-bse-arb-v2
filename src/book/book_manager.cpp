#include "book/book_manager.hpp"
#include <sstream>

namespace hft {

BestQuote BookManager::on_event(const BookEvent& ev) {
    std::string k = key(ev.venue, ev.instrument_id);
    auto it = books_.find(k);
    if (it == books_.end()) {
        it = books_.emplace(k, OrderBook(ev.venue, ev.instrument_id)).first;
    }
    it->second.apply(ev);
    return it->second.best_quote(ev.ts_ingest_ns);
}

std::optional<BestQuote> BookManager::quote(Venue venue, uint32_t instrument_id, uint64_t now_ns) const {
    auto it = books_.find(key(venue, instrument_id));
    if (it == books_.end()) return std::nullopt;
    return it->second.best_quote(now_ns);
}

OrderBook* BookManager::book(Venue venue, uint32_t instrument_id) {
    auto it = books_.find(key(venue, instrument_id));
    if (it == books_.end()) return nullptr;
    return &it->second;
}

const OrderBook* BookManager::book(Venue venue, uint32_t instrument_id) const {
    auto it = books_.find(key(venue, instrument_id));
    if (it == books_.end()) return nullptr;
    return &it->second;
}

bool BookManager::invariant_ok(std::string* reason) const {
    for (const auto& [k, b] : books_) {
        std::string r;
        if (!b.invariant_ok(&r)) {
            if (reason) *reason = k + ": " + r;
            return false;
        }
    }
    if (reason) *reason = "OK";
    return true;
}

std::string BookManager::key(Venue venue, uint32_t instrument_id) {
    std::ostringstream oss;
    oss << static_cast<int>(venue) << ':' << instrument_id;
    return oss.str();
}

} // namespace hft
