#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include "common/ids.hpp"
#include "oms/fill.hpp"
#include "oms/gateway.hpp"
#include "risk/position_book.hpp"

namespace hft {

class OrderManager {
public:
    explicit OrderManager(PositionBook& positions) : positions_(positions), ids_("SIM") {}

    Order& submit(const OrderIntent& intent, IOrderGateway& gateway, uint64_t now_ns);
    void on_report(const ExecutionReport& report);

    const std::unordered_map<std::string, Order>& orders() const { return orders_; }
    const std::vector<Fill>& fills() const { return fills_; }
    std::vector<Fill> take_new_fills();
    bool known_order(const std::string& client_order_id) const;

private:
    PositionBook& positions_;
    IdGenerator ids_;
    std::unordered_map<std::string, Order> orders_;
    std::vector<Fill> fills_;
    size_t consumed_fills_{0};
};

} // namespace hft
