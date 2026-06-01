#include "md/market_data_handler.hpp"

namespace hft {

MarketDataHandler::MarketDataHandler(std::string replay_path) : reader_(std::move(replay_path)) {}

std::optional<MdResult> MarketDataHandler::next() {
    auto pkt = reader_.next_packet();
    if (!pkt) return std::nullopt;

    MdResult out;
    out.sequence_status = seq_.on_packet(pkt->venue, pkt->channel_id, pkt->sequence_no);
    if (out.sequence_status == SequenceStatus::Duplicate) {
        out.warning = "duplicate packet ignored";
        return out;
    }

    auto ev = parser_.parse(*pkt);
    if (!ev) {
        out.warning = "parse failure";
        return out;
    }

    if (out.sequence_status == SequenceStatus::Gap) {
        health_.mark_unhealthy(ev->venue, ev->instrument_id, "SEQUENCE_GAP");
        out.warning = "sequence gap";
    } else {
        health_.on_event(ev->venue, ev->instrument_id, ev->ts_ingest_ns);
    }
    out.event = *ev;
    return out;
}

} // namespace hft
