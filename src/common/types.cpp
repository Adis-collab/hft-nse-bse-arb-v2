#include "common/types.hpp"
#include "common/csv.hpp"
#include <algorithm>
#include <chrono>

namespace hft {

std::string to_string(Venue v) {
    switch (v) {
        case Venue::NSE: return "NSE";
        case Venue::BSE: return "BSE";
        default: return "UNKNOWN";
    }
}

std::string to_string(Side s) {
    switch (s) {
        case Side::Buy: return "BUY";
        case Side::Sell: return "SELL";
        default: return "UNKNOWN";
    }
}

std::string to_string(EventType e) {
    switch (e) {
        case EventType::Add: return "ADD";
        case EventType::Modify: return "MODIFY";
        case EventType::Cancel: return "CANCEL";
        case EventType::Trade: return "TRADE";
        case EventType::Heartbeat: return "HEARTBEAT";
        case EventType::Reset: return "RESET";
        default: return "UNKNOWN";
    }
}

std::string to_string(OrderType t) {
    return t == OrderType::Limit ? "LIMIT" : "MARKET";
}

std::string to_string(TimeInForce t) {
    return t == TimeInForce::IOC ? "IOC" : "DAY";
}

std::string to_string(OrderRole r) {
    switch (r) {
        case OrderRole::AlphaBuy: return "ALPHA_BUY";
        case OrderRole::AlphaSell: return "ALPHA_SELL";
        case OrderRole::Hedge: return "HEDGE";
        case OrderRole::Passive: return "PASSIVE";
        default: return "UNKNOWN";
    }
}

Venue venue_from_string(const std::string& input) {
    std::string s = upper(trim(input));
    if (s == "NSE") return Venue::NSE;
    if (s == "BSE") return Venue::BSE;
    return Venue::Unknown;
}

Side side_from_string(const std::string& input) {
    std::string s = upper(trim(input));
    if (s == "BUY" || s == "B") return Side::Buy;
    if (s == "SELL" || s == "S") return Side::Sell;
    return Side::Unknown;
}

EventType event_type_from_string(const std::string& input) {
    std::string s = upper(trim(input));
    if (s == "ADD" || s == "A") return EventType::Add;
    if (s == "MODIFY" || s == "MOD" || s == "M") return EventType::Modify;
    if (s == "CANCEL" || s == "CXL" || s == "C") return EventType::Cancel;
    if (s == "TRADE" || s == "T") return EventType::Trade;
    if (s == "HEARTBEAT" || s == "HB") return EventType::Heartbeat;
    if (s == "RESET" || s == "R") return EventType::Reset;
    return EventType::Unknown;
}

OrderRole order_role_from_string(const std::string& input) {
    std::string s = upper(trim(input));
    if (s == "ALPHA_BUY") return OrderRole::AlphaBuy;
    if (s == "ALPHA_SELL") return OrderRole::AlphaSell;
    if (s == "HEDGE") return OrderRole::Hedge;
    if (s == "PASSIVE") return OrderRole::Passive;
    return OrderRole::Unknown;
}

Side opposite(Side s) {
    if (s == Side::Buy) return Side::Sell;
    if (s == Side::Sell) return Side::Buy;
    return Side::Unknown;
}

Venue other_venue(Venue v) {
    if (v == Venue::NSE) return Venue::BSE;
    if (v == Venue::BSE) return Venue::NSE;
    return Venue::Unknown;
}

uint64_t now_ns() {
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
}

} // namespace hft
