#pragma once

#include <cstdint>
#include <string>
#include "common/types.hpp"
#include "oms/order_intent.hpp"

namespace hft {

enum class OrderState { New, Sent, Acked, Partial, Filled, Canceled, Rejected, Expired };

inline std::string to_string(OrderState s) {
    switch (s) {
        case OrderState::New: return "NEW";
        case OrderState::Sent: return "SENT";
        case OrderState::Acked: return "ACKED";
        case OrderState::Partial: return "PARTIAL";
        case OrderState::Filled: return "FILLED";
        case OrderState::Canceled: return "CANCELED";
        case OrderState::Rejected: return "REJECTED";
        case OrderState::Expired: return "EXPIRED";
        default: return "UNKNOWN";
    }
}

struct Order {
    OrderIntent intent;
    std::string client_order_id;
    std::string exchange_order_id;
    int64_t filled_qty{};
    OrderState state{OrderState::New};
    uint64_t ts_sent_ns{};
    uint64_t ts_ack_ns{};
    uint64_t ts_done_ns{};
    std::string err_code;
};

} // namespace hft
