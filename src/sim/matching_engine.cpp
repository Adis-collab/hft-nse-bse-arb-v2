#include "sim/matching_engine.hpp"
#include "common/types.hpp"

namespace hft {

std::vector<ExecutionReport> MatchingEngine::send(const Order& order) {
    std::vector<ExecutionReport> reports;
    uint64_t ts = now_ns() + static_cast<uint64_t>(scenario_.extra_latency_us) * 1000ULL;

    auto make = [&](ExecType t, int64_t px, int64_t qty, std::string reason = "") {
        return ExecutionReport{t, order.client_order_id, order.intent.parent_arb_id, order.intent.instrument_id,
                               order.intent.venue, order.intent.side, px, qty, ts, std::move(reason)};
    };

    if (scenario_.disconnected(order.intent.venue)) {
        reports.push_back(make(ExecType::Reject, order.intent.price_paise, 0, "GATEWAY_DISCONNECT"));
        return reports;
    }
    if (scenario_.should_reject(order.intent.role)) {
        reports.push_back(make(ExecType::Reject, order.intent.price_paise, 0, "SCENARIO_REJECT"));
        return reports;
    }

    Order capped = order;
    capped.intent.qty = scenario_.cap_fill_qty(order.intent.role, order.intent.qty);

    OrderBook* book = books_.book(capped.intent.venue, capped.intent.instrument_id);
    if (!book) {
        reports.push_back(make(ExecType::Reject, order.intent.price_paise, 0, "NO_BOOK"));
        return reports;
    }

    auto level_fills = book->match_ioc(capped.intent.side, capped.intent.price_paise, capped.intent.qty);
    int64_t filled = 0;

    if (!scenario_.delayed_ack) reports.push_back(make(ExecType::Ack, order.intent.price_paise, 0, ""));
    for (const auto& lf : level_fills) {
        filled += lf.qty;
        reports.push_back(make(ExecType::Fill, lf.price_paise, lf.qty, ""));
    }
    if (scenario_.delayed_ack) reports.push_back(make(ExecType::Ack, order.intent.price_paise, 0, "DELAYED_ACK"));

    if (filled < order.intent.qty) {
        reports.push_back(make(ExecType::Expire, order.intent.price_paise, order.intent.qty - filled, "IOC_REMAINDER_EXPIRED"));
    }
    return reports;
}

} // namespace hft
