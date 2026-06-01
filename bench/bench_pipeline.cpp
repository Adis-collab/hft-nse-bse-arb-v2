#include "book/book_manager.hpp"
#include "metrics/latency_recorder.hpp"
#include "strategy/arbitrage_strategy.hpp"
#include <iostream>

using namespace hft;

int main() {
    LatencyRecorder::instance().clear();
    BookManager bm;
    CostModel costs{5,5,5,5,10,5};
    ArbitrageStrategy strat;
    for (int i = 0; i < 10000; ++i) {
        uint64_t ts = 1000000000ULL + static_cast<uint64_t>(i) * 1000ULL;
        {
            StageTimer t("book_update");
            bm.on_event(BookEvent{ts, ts, Venue::NSE, 1, 1594, EventType::Add, static_cast<uint64_t>(100000+i), Side::Buy, 110050, 10, ""});
            bm.on_event(BookEvent{ts, ts, Venue::NSE, 1, 1594, EventType::Add, static_cast<uint64_t>(200000+i), Side::Sell, 110080, 10, ""});
            bm.on_event(BookEvent{ts, ts, Venue::BSE, 1, 500209, EventType::Add, static_cast<uint64_t>(300000+i), Side::Buy, 109950, 10, ""});
            bm.on_event(BookEvent{ts, ts, Venue::BSE, 1, 500209, EventType::Add, static_cast<uint64_t>(400000+i), Side::Sell, 110000, 10, ""});
        }
        auto nse = bm.quote(Venue::NSE, 1, ts).value();
        auto bse = bm.quote(Venue::BSE, 1, ts).value();
        {
            StageTimer t("strategy");
            auto d = strat.evaluate(nse, bse, costs, ts);
            (void)d;
        }
    }
    std::cout << LatencyRecorder::instance().summary_csv();
    return 0;
}
