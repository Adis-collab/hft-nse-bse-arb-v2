#include "oms/order_manager.hpp"
#include "common/types.hpp"
#include <stdexcept>

namespace hft {

Order& OrderManager::submit(const OrderIntent& intent, IOrderGateway& gateway, uint64_t now_ns) {
    Order o;
    o.intent = intent;
    o.client_order_id = ids_.next_order(intent.parent_arb_id, intent.venue, intent.side, intent.role);
    o.state = OrderState::Sent;
    o.ts_sent_ns = now_ns;
    auto [it, inserted] = orders_.emplace(o.client_order_id, o);
    (void)inserted;

    auto reports = gateway.send(it->second);
    for (auto& r : reports) {
        if (r.client_order_id.empty()) r.client_order_id = it->second.client_order_id;
        if (r.parent_arb_id.empty()) r.parent_arb_id = intent.parent_arb_id;
        on_report(r);
    }
    return it->second;
}

void OrderManager::on_report(const ExecutionReport& report) {
    auto it = orders_.find(report.client_order_id);
    if (it == orders_.end()) return;
    Order& o = it->second;
    switch (report.type) {
        case ExecType::Ack:
            if (o.state == OrderState::Sent) {
                o.state = OrderState::Acked;
                o.ts_ack_ns = report.ts_ns;
            }
            break;
        case ExecType::Fill: {
            o.filled_qty += report.qty;
            if (o.filled_qty >= o.intent.qty) {
                o.state = OrderState::Filled;
                o.ts_done_ns = report.ts_ns;
            } else {
                o.state = OrderState::Partial;
            }
            Fill f;
            f.parent_arb_id = o.intent.parent_arb_id;
            f.client_order_id = o.client_order_id;
            f.instrument_id = o.intent.instrument_id;
            f.venue = o.intent.venue;
            f.side = o.intent.side;
            f.fill_px_paise = report.price_paise;
            f.fill_qty = report.qty;
            f.ts_fill_ns = report.ts_ns;
            fills_.push_back(f);
            positions_.apply_fill(f.instrument_id, f.side, f.fill_qty, f.fill_px_paise);
            break;
        }
        case ExecType::Reject:
            if (o.filled_qty > 0) o.state = OrderState::Partial;
            else o.state = OrderState::Rejected;
            o.err_code = report.reason;
            o.ts_done_ns = report.ts_ns;
            break;
        case ExecType::Expire:
            if (o.filled_qty > 0) o.state = OrderState::Partial;
            else o.state = OrderState::Expired;
            o.err_code = report.reason.empty() ? "IOC_EXPIRED" : report.reason;
            o.ts_done_ns = report.ts_ns;
            break;
        case ExecType::Cancel:
            o.state = OrderState::Canceled;
            o.ts_done_ns = report.ts_ns;
            break;
    }
}

std::vector<Fill> OrderManager::take_new_fills() {
    std::vector<Fill> out;
    for (size_t i = consumed_fills_; i < fills_.size(); ++i) out.push_back(fills_[i]);
    consumed_fills_ = fills_.size();
    return out;
}

bool OrderManager::known_order(const std::string& client_order_id) const {
    return orders_.find(client_order_id) != orders_.end();
}

} // namespace hft
