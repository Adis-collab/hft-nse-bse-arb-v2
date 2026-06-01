#pragma once

#include <cstdint>
#include "common/types.hpp"

namespace hft {

struct BestQuote {
    Venue venue{Venue::Unknown};
    uint32_t instrument_id{};
    int64_t bid_px_paise{};
    int64_t bid_qty{};
    int64_t ask_px_paise{};
    int64_t ask_qty{};
    uint32_t book_age_us{UINT32_MAX};

    bool valid() const {
        return venue != Venue::Unknown && instrument_id != 0 && bid_px_paise > 0 && ask_px_paise > 0 && bid_qty > 0 && ask_qty > 0;
    }
};

} // namespace hft
