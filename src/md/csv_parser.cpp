#include "md/csv_parser.hpp"
#include "common/csv.hpp"
#include "common/types.hpp"

namespace hft {

std::optional<BookEvent> CsvParser::parse(const RawPacket& pkt) const {
    auto c = split_csv(pkt.payload_bytes);
    if (c.size() < 13) return std::nullopt;
    BookEvent ev;
    ev.ts_exchange_ns = parse_u64(c[0]);
    ev.ts_ingest_ns = parse_u64(c[1]);
    ev.venue = venue_from_string(c[2]);
    ev.instrument_id = static_cast<uint32_t>(parse_u64(c[6]));
    ev.native_token = static_cast<uint32_t>(parse_u64(c[7]));
    ev.event_type = event_type_from_string(c[8]);
    ev.order_id = parse_u64(c[9]);
    ev.side = side_from_string(c[10]);
    ev.price_paise = parse_i64(c[11]);
    ev.qty = parse_i64(c[12]);
    ev.native_payload = pkt.payload_bytes;
    return ev;
}

} // namespace hft
