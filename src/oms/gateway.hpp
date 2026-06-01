#pragma once

#include <string>
#include <vector>
#include "oms/fill.hpp"
#include "oms/order_state.hpp"

namespace hft {

enum class ExecType { Ack, Fill, Reject, Expire, Cancel };

struct ExecutionReport {
    ExecType type{ExecType::Ack};
    std::string client_order_id;
    std::string parent_arb_id;
    uint32_t instrument_id{};
    Venue venue{Venue::Unknown};
    Side side{Side::Unknown};
    int64_t price_paise{};
    int64_t qty{};
    uint64_t ts_ns{};
    std::string reason;
};

class IOrderGateway {
public:
    virtual ~IOrderGateway() = default;
    virtual std::vector<ExecutionReport> send(const Order& order) = 0;
};

} // namespace hft
