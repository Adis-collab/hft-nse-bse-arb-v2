#pragma once

#include <optional>
#include <string>
#include "md/book_event.hpp"
#include "md/csv_parser.hpp"
#include "md/feed_health.hpp"
#include "md/replay_reader.hpp"
#include "md/sequence_tracker.hpp"

namespace hft {

struct MdResult {
    std::optional<BookEvent> event;
    SequenceStatus sequence_status{SequenceStatus::Ok};
    std::string warning;
};

class MarketDataHandler {
public:
    explicit MarketDataHandler(std::string replay_path);
    std::optional<MdResult> next();
    FeedHealth& health() { return health_; }
    const FeedHealth& health() const { return health_; }

private:
    ReplayReader reader_;
    CsvParser parser_;
    SequenceTracker seq_;
    FeedHealth health_;
};

} // namespace hft
