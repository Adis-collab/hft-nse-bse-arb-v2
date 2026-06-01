#pragma once

#include <cstdint>
#include <string>

namespace hft {

enum class Venue { NSE, BSE, Unknown };
enum class Side { Buy, Sell, Unknown };
enum class EventType { Add, Modify, Cancel, Trade, Heartbeat, Reset, Unknown };
enum class OrderType { Limit, Market };
enum class TimeInForce { Day, IOC };
enum class OrderRole { AlphaBuy, AlphaSell, Hedge, Passive, Unknown };

std::string to_string(Venue v);
std::string to_string(Side s);
std::string to_string(EventType e);
std::string to_string(OrderType t);
std::string to_string(TimeInForce t);
std::string to_string(OrderRole r);

Venue venue_from_string(const std::string& s);
Side side_from_string(const std::string& s);
EventType event_type_from_string(const std::string& s);
OrderRole order_role_from_string(const std::string& s);

Side opposite(Side s);
Venue other_venue(Venue v);

uint64_t now_ns();

} // namespace hft
