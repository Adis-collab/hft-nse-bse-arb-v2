#include "book/book_manager.hpp"
#include "hedge/hedge_policy.hpp"
#include "md/csv_parser.hpp"
#include "md/sequence_tracker.hpp"
#include "oms/reconciler.hpp"
#include "risk/guards.hpp"
#include "risk/risk_engine.hpp"
#include "sim/simulator.hpp"
#include "strategy/arbitrage_strategy.hpp"
#include "concurrency/spsc_ring_buffer.hpp"
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

using namespace hft;

static int failures = 0;

#define EXPECT_TRUE(x) do { if (!(x)) { std::cerr << "FAIL " << __FILE__ << ':' << __LINE__ << " EXPECT_TRUE(" #x ")\n"; ++failures; } } while(0)
#define EXPECT_FALSE(x) EXPECT_TRUE(!(x))
#define EXPECT_EQ(a,b) do { auto _a=(a); auto _b=(b); if (!(_a == _b)) { std::cerr << "FAIL " << __FILE__ << ':' << __LINE__ << " EXPECT_EQ(" #a "," #b ") got " << _a << " vs " << _b << "\n"; ++failures; } } while(0)

static BookEvent ev(Venue v, uint64_t seq, EventType t, uint64_t oid, Side s, int64_t px, int64_t qty) {
    return BookEvent{1000000000ULL + seq * 100ULL, 1000000100ULL + seq * 100ULL, v, 1, v == Venue::NSE ? 1594u : 500209u, t, oid, s, px, qty, ""};
}


void test_spsc_ring_buffer_basic() {
    hft::concurrency::SpscRingBuffer<int, 4> q;
    EXPECT_TRUE(q.empty());
    EXPECT_TRUE(q.try_push(10));
    EXPECT_TRUE(q.try_push(20));
    EXPECT_TRUE(q.try_push(30));
    EXPECT_TRUE(q.full()); // raw capacity 4 => usable capacity 3
    EXPECT_FALSE(q.try_push(40));

    int out = 0;
    EXPECT_TRUE(q.try_pop(out));
    EXPECT_EQ(out, 10);
    EXPECT_TRUE(q.try_pop(out));
    EXPECT_EQ(out, 20);
    EXPECT_TRUE(q.try_push(40)); // verifies wraparound
    EXPECT_TRUE(q.try_pop(out));
    EXPECT_EQ(out, 30);
    EXPECT_TRUE(q.try_pop(out));
    EXPECT_EQ(out, 40);
    EXPECT_TRUE(q.empty());
    EXPECT_FALSE(q.try_pop(out));
}

void test_spsc_ring_buffer_move_only() {
    hft::concurrency::SpscRingBuffer<std::unique_ptr<int>, 8> q;
    EXPECT_TRUE(q.try_push(std::make_unique<int>(42)));
    std::unique_ptr<int> out;
    EXPECT_TRUE(q.try_pop(out));
    EXPECT_TRUE(static_cast<bool>(out));
    EXPECT_EQ(*out, 42);
}

void test_sequence_tracker() {
    SequenceTracker st;
    EXPECT_TRUE(st.on_packet(Venue::NSE, 1, 1) == SequenceStatus::Ok);
    EXPECT_TRUE(st.on_packet(Venue::NSE, 1, 2) == SequenceStatus::Ok);
    EXPECT_TRUE(st.on_packet(Venue::NSE, 1, 2) == SequenceStatus::Duplicate);
    EXPECT_TRUE(st.on_packet(Venue::NSE, 1, 5) == SequenceStatus::Gap);
}

void test_csv_parser() {
    RawPacket pkt;
    pkt.payload_bytes = "100,200,NSE,1,1,ADD,1,1594,ADD,101,BUY,110050,100";
    CsvParser p;
    auto ev = p.parse(pkt);
    EXPECT_TRUE(ev.has_value());
    EXPECT_TRUE(ev->venue == Venue::NSE);
    EXPECT_TRUE(ev->side == Side::Buy);
    EXPECT_EQ(ev->price_paise, 110050);
}

void test_order_book() {
    OrderBook b(Venue::NSE, 1);
    b.apply(ev(Venue::NSE, 1, EventType::Add, 1, Side::Buy, 100, 10));
    b.apply(ev(Venue::NSE, 2, EventType::Add, 2, Side::Sell, 105, 20));
    auto q = b.best_quote(1000000500);
    EXPECT_EQ(q.bid_px_paise, 100);
    EXPECT_EQ(q.ask_px_paise, 105);
    b.modify(1, Side::Buy, 101, 15);
    EXPECT_EQ(b.best_quote(1000000600).bid_px_paise, 101);
    b.cancel(2);
    EXPECT_EQ(b.best_quote(1000000700).ask_qty, 0);
    std::string reason;
    EXPECT_TRUE(b.invariant_ok(&reason));
}

void test_strategy() {
    BestQuote nse{Venue::NSE, 1, 110050, 100, 110080, 100, 50};
    BestQuote bse{Venue::BSE, 1, 109950, 100, 110000, 100, 50};
    CostModel costs{5,5,5,5,10,5};
    StrategyConfig cfg{5,100,250,1};
    ArbitrageStrategy s(cfg);
    auto d = s.evaluate(nse, bse, costs, 1000);
    EXPECT_TRUE(d.should_trade);
    EXPECT_TRUE(d.opportunity.buy_venue == Venue::BSE);
    EXPECT_TRUE(d.opportunity.sell_venue == Venue::NSE);
    EXPECT_EQ(d.opportunity.gross_spread_paise, 50);
    EXPECT_EQ(d.opportunity.net_edge_paise, 10);

    CostModel too_expensive{20,20,20,20,20,5};
    auto d2 = s.evaluate(nse, bse, too_expensive, 1000);
    EXPECT_FALSE(d2.should_trade);
    EXPECT_EQ(d2.reason, std::string("EDGE_TOO_SMALL"));
}

void test_risk() {
    RiskConfig cfg;
    PositionBook pos;
    RiskEngine r(cfg, pos);
    OrderIntent o{"P",1,Venue::NSE,Side::Buy,OrderType::Limit,TimeInForce::IOC,OrderRole::AlphaBuy,110080,10};
    BestQuote q{Venue::NSE,1,110050,100,110080,100,0};
    auto d = r.check_order(o, q, 1000000000ULL);
    EXPECT_TRUE(d.approved);
    o.qty = 10000;
    auto d2 = r.check_order(o, q, 1000000001ULL);
    EXPECT_EQ(d2.rule, std::string("MAX_QTY"));
}

void test_legging_hedge_long() {
    PairTrade p;
    p.parent_id = "P";
    p.instrument_id = 1;
    p.intended_buy_venue = Venue::BSE;
    p.intended_sell_venue = Venue::NSE;
    Fill buy{"P","B",1,Venue::BSE,Side::Buy,110000,100,1000};
    p.apply_fill(buy);
    p.mark_leg_failed("NSE_REJECT", 1100);
    EXPECT_EQ(p.residual(), 100);
    RiskConfig cfg;
    HedgePolicy hp(cfg);
    BestQuote nse{Venue::NSE,1,110040,100,110080,100,0};
    BestQuote bse{Venue::BSE,1,109980,100,110010,100,0};
    auto h = hp.next_order(p, nse, bse);
    EXPECT_TRUE(h.should_send);
    EXPECT_TRUE(h.intent.side == Side::Sell);
    EXPECT_EQ(h.intent.qty, 100);
}

void test_legging_hedge_short() {
    PairTrade p;
    p.parent_id = "P";
    p.instrument_id = 1;
    p.intended_buy_venue = Venue::BSE;
    p.intended_sell_venue = Venue::NSE;
    Fill sell{"P","S",1,Venue::NSE,Side::Sell,110050,100,1000};
    p.apply_fill(sell);
    p.mark_leg_failed("BSE_REJECT", 1100);
    EXPECT_EQ(p.residual(), -100);
    RiskConfig cfg;
    HedgePolicy hp(cfg);
    BestQuote nse{Venue::NSE,1,110040,100,110080,100,0};
    BestQuote bse{Venue::BSE,1,109980,100,110020,100,0};
    auto h = hp.next_order(p, nse, bse);
    EXPECT_TRUE(h.should_send);
    EXPECT_TRUE(h.intent.side == Side::Buy);
    EXPECT_EQ(h.intent.qty, 100);
}

void test_simulator_clean() {
    CostModel costs{5,5,5,5,10,5};
    RiskConfig risk;
    StrategyConfig scfg{5,100,250,1};
    Simulator sim(costs, risk, scfg, ScenarioConfig::from_name("clean"));
    auto res = sim.run_replay("data/sample_day.csv", "data/out/test_clean", 1);
    EXPECT_EQ(res.final_state, std::string("FLAT"));
    EXPECT_TRUE(res.trades >= 1);
}

void test_reconciler_and_guards() {
    PositionBook p;
    OrderManager oms(p);
    Reconciler rec;
    Fill f{"P","UNKNOWN",1,Venue::NSE,Side::Buy,100,1,1};
    auto evt = rec.on_external_fill(f, oms);
    EXPECT_TRUE(evt.has_value());
    EXPECT_EQ(evt->rule_name, std::string("ORPHAN_FILL"));
    EXPECT_TRUE(ClockSkewGuard::severe_skew(1000, 5000000, 100000));
    EXPECT_TRUE(EodGuard::should_flatten(1000, 1500, 600, 5));
}

int main() {
    test_spsc_ring_buffer_basic();
    test_spsc_ring_buffer_move_only();
    test_sequence_tracker();
    test_csv_parser();
    test_order_book();
    test_strategy();
    test_risk();
    test_legging_hedge_long();
    test_legging_hedge_short();
    test_simulator_clean();
    test_reconciler_and_guards();
    if (failures) {
        std::cerr << failures << " tests failed\n";
        return 1;
    }
    std::cout << "all tests passed\n";
    return 0;
}
