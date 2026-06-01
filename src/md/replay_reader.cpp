#include "md/replay_reader.hpp"
#include "common/csv.hpp"
#include "common/types.hpp"
#include <stdexcept>

namespace hft {

ReplayReader::ReplayReader(std::string path) : path_(std::move(path)) {
    in_.open(path_);
    if (!in_) throw std::runtime_error("cannot open replay file: " + path_);
}

std::optional<RawPacket> ReplayReader::next_packet() {
    std::string line;
    while (std::getline(in_, line)) {
        line = trim(line);
        if (line.empty()) continue;
        if (!header_skipped_) {
            header_skipped_ = true;
            if (line.find("ts_exchange_ns") != std::string::npos) continue;
        }
        auto cols = split_csv(line);
        if (cols.size() < 6) continue;
        RawPacket pkt;
        pkt.ts_ingest_ns = parse_u64(cols[1]);
        pkt.venue = venue_from_string(cols[2]);
        pkt.channel_id = static_cast<uint32_t>(parse_u64(cols[3]));
        pkt.sequence_no = parse_u64(cols[4]);
        pkt.msg_type = cols[5];
        pkt.payload_bytes = line;
        return pkt;
    }
    return std::nullopt;
}

} // namespace hft
