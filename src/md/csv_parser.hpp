#pragma once

#include <optional>
#include "md/book_event.hpp"
#include "md/packet.hpp"

namespace hft {

class CsvParser {
public:
    std::optional<BookEvent> parse(const RawPacket& pkt) const;
};

} // namespace hft
