#pragma once

#include <cstdint>
#include <cstdlib>

namespace hft {

struct ClockSkewGuard {
    static bool severe_skew(uint64_t exchange_ts_ns, uint64_t ingest_ts_ns, uint64_t max_abs_skew_ns) {
        uint64_t diff = exchange_ts_ns > ingest_ts_ns ? exchange_ts_ns - ingest_ts_ns : ingest_ts_ns - exchange_ts_ns;
        return diff > max_abs_skew_ns;
    }
};

struct EodGuard {
    static bool should_flatten(uint64_t now_ns, uint64_t session_end_ns, uint64_t flatten_window_ns, int64_t residual_qty) {
        return residual_qty != 0 && session_end_ns > now_ns && (session_end_ns - now_ns) <= flatten_window_ns;
    }
};

} // namespace hft
