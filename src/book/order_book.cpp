#include "book/order_book.hpp"
#include <algorithm>
#include <cstdlib>
#include <sstream>

namespace hft {

void OrderBook::apply(const BookEvent& ev) {
    venue_ = ev.venue;
    instrument_id_ = ev.instrument_id;
    last_update_ns_ = ev.ts_ingest_ns;
    switch (ev.event_type) {
        case EventType::Add:
            add(ev.order_id, ev.side, ev.price_paise, ev.qty);
            break;
        case EventType::Modify:
            modify(ev.order_id, ev.side, ev.price_paise, ev.qty);
            break;
        case EventType::Cancel:
            cancel(ev.order_id);
            break;
        case EventType::Trade:
            trade(ev.order_id, ev.side, ev.price_paise, ev.qty);
            break;
        case EventType::Reset:
            clear();
            break;
        case EventType::Heartbeat:
        case EventType::Unknown:
            break;
    }
}

void OrderBook::add(uint64_t id, Side side, int64_t px, int64_t qty) {
    if (id == 0 || side == Side::Unknown || px <= 0 || qty <= 0) return;
    if (orders_.find(id) != orders_.end()) cancel(id);
    orders_[id] = RestingOrder{side, px, qty};
    if (side == Side::Buy) bids_[px] += qty;
    else asks_[px] += qty;
}

void OrderBook::modify(uint64_t id, Side side, int64_t new_px, int64_t new_qty) {
    auto it = orders_.find(id);
    if (it == orders_.end()) {
        add(id, side, new_px, new_qty);
        return;
    }
    Side old_side = it->second.side;
    remove_from_level(old_side, it->second.price_paise, it->second.qty);
    if (new_qty <= 0) {
        orders_.erase(it);
        return;
    }
    Side final_side = side == Side::Unknown ? old_side : side;
    it->second = RestingOrder{final_side, new_px, new_qty};
    if (final_side == Side::Buy) bids_[new_px] += new_qty;
    else asks_[new_px] += new_qty;
}

void OrderBook::cancel(uint64_t id) {
    auto it = orders_.find(id);
    if (it == orders_.end()) return;
    remove_from_level(it->second.side, it->second.price_paise, it->second.qty);
    orders_.erase(it);
}

void OrderBook::trade(uint64_t resting_id, Side side, int64_t px, int64_t qty) {
    if (qty <= 0) return;
    auto it = orders_.find(resting_id);
    if (it != orders_.end()) {
        int64_t consumed = std::min(qty, it->second.qty);
        remove_from_level(it->second.side, it->second.price_paise, consumed);
        it->second.qty -= consumed;
        if (it->second.qty <= 0) orders_.erase(it);
        return;
    }
    if (side != Side::Unknown && px > 0) {
        remove_from_level(side, px, qty);
    }
}

std::vector<LevelFill> OrderBook::match_ioc(Side aggressor_side, int64_t limit_px, int64_t qty) {
    std::vector<LevelFill> fills;
    if (qty <= 0 || limit_px <= 0) return fills;

    if (aggressor_side == Side::Buy) {
        while (qty > 0 && !asks_.empty()) {
            auto it = asks_.begin();
            int64_t px = it->first;
            if (px > limit_px) break;
            int64_t fill_qty = std::min(qty, it->second);
            fills.push_back(LevelFill{px, fill_qty});
            consume_orders_at_price(Side::Sell, px, fill_qty);
            qty -= fill_qty;
        }
    } else if (aggressor_side == Side::Sell) {
        while (qty > 0 && !bids_.empty()) {
            auto it = bids_.begin();
            int64_t px = it->first;
            if (px < limit_px) break;
            int64_t fill_qty = std::min(qty, it->second);
            fills.push_back(LevelFill{px, fill_qty});
            consume_orders_at_price(Side::Buy, px, fill_qty);
            qty -= fill_qty;
        }
    }
    return fills;
}

BestQuote OrderBook::best_quote(uint64_t now_ns) const {
    BestQuote q;
    q.venue = venue_;
    q.instrument_id = instrument_id_;
    if (!bids_.empty()) {
        q.bid_px_paise = bids_.begin()->first;
        q.bid_qty = bids_.begin()->second;
    }
    if (!asks_.empty()) {
        q.ask_px_paise = asks_.begin()->first;
        q.ask_qty = asks_.begin()->second;
    }
    if (last_update_ns_ > 0 && now_ns >= last_update_ns_) q.book_age_us = static_cast<uint32_t>((now_ns - last_update_ns_) / 1000ULL);
    else q.book_age_us = 0;
    return q;
}

bool OrderBook::invariant_ok(std::string* reason) const {
    auto fail = [&](const std::string& r) {
        if (reason) *reason = r;
        return false;
    };
    for (const auto& [px, qty] : bids_) {
        if (px <= 0 || qty <= 0) return fail("bad bid level");
    }
    for (const auto& [px, qty] : asks_) {
        if (px <= 0 || qty <= 0) return fail("bad ask level");
    }
    std::unordered_map<int64_t, int64_t> bid_sum;
    std::unordered_map<int64_t, int64_t> ask_sum;
    for (const auto& [id, o] : orders_) {
        (void)id;
        if (o.qty <= 0 || o.price_paise <= 0) return fail("bad order index entry");
        if (o.side == Side::Buy) bid_sum[o.price_paise] += o.qty;
        else if (o.side == Side::Sell) ask_sum[o.price_paise] += o.qty;
    }
    for (const auto& [px, qty] : bids_) {
        auto it = bid_sum.find(px);
        if (it == bid_sum.end() || it->second != qty) return fail("bid level/index mismatch");
    }
    for (const auto& [px, qty] : asks_) {
        auto it = ask_sum.find(px);
        if (it == ask_sum.end() || it->second != qty) return fail("ask level/index mismatch");
    }
    if (reason) *reason = "OK";
    return true;
}

bool OrderBook::crossed() const {
    return !bids_.empty() && !asks_.empty() && bids_.begin()->first >= asks_.begin()->first;
}

int64_t OrderBook::depth_at(Side side, int64_t price_paise) const {
    if (side == Side::Buy) {
        auto it = bids_.find(price_paise);
        return it == bids_.end() ? 0 : it->second;
    }
    if (side == Side::Sell) {
        auto it = asks_.find(price_paise);
        return it == asks_.end() ? 0 : it->second;
    }
    return 0;
}

int64_t OrderBook::total_depth(Side side) const {
    int64_t total = 0;
    if (side == Side::Buy) {
        for (const auto& [px, qty] : bids_) { (void)px; total += qty; }
    } else if (side == Side::Sell) {
        for (const auto& [px, qty] : asks_) { (void)px; total += qty; }
    }
    return total;
}

void OrderBook::remove_from_level(Side side, int64_t px, int64_t qty) {
    if (qty <= 0) return;
    if (side == Side::Buy) {
        auto it = bids_.find(px);
        if (it == bids_.end()) return;
        it->second -= qty;
        if (it->second <= 0) bids_.erase(it);
    } else if (side == Side::Sell) {
        auto it = asks_.find(px);
        if (it == asks_.end()) return;
        it->second -= qty;
        if (it->second <= 0) asks_.erase(it);
    }
}

void OrderBook::consume_orders_at_price(Side resting_side, int64_t px, int64_t qty_to_consume) {
    int64_t remaining = qty_to_consume;
    std::vector<uint64_t> erase_ids;
    for (auto& [id, o] : orders_) {
        if (remaining <= 0) break;
        if (o.side != resting_side || o.price_paise != px) continue;
        int64_t take = std::min(remaining, o.qty);
        o.qty -= take;
        remaining -= take;
        if (o.qty <= 0) erase_ids.push_back(id);
    }
    remove_from_level(resting_side, px, qty_to_consume - remaining);
    for (uint64_t id : erase_ids) orders_.erase(id);
}

void OrderBook::clear() {
    bids_.clear();
    asks_.clear();
    orders_.clear();
}

} // namespace hft
