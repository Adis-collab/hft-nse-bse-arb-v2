#pragma once

#include <optional>
#include "oms/fill.hpp"
#include "oms/order_manager.hpp"
#include "risk/risk_event.hpp"

namespace hft {

class Reconciler {
public:
    std::optional<RiskEvent> on_external_fill(const Fill& f, const OrderManager& oms) const {
        if (!oms.known_order(f.client_order_id)) {
            return RiskEvent{f.ts_fill_ns, "HIGH", "ORPHAN_FILL", f.instrument_id, "{\"client_order_id\":\"" + f.client_order_id + "\"}"};
        }
        return std::nullopt;
    }
};

} // namespace hft
