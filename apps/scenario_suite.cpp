#include "book/book_manager.hpp"
#include "hedge/hedge_policy.hpp"
#include "md/sequence_tracker.hpp"
#include "oms/reconciler.hpp"
#include "risk/guards.hpp"
#include "risk/risk_engine.hpp"
#include "sim/simulator.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace hft;

static std::string arg_value(int argc, char** argv, const std::string& key, const std::string& def = "") {
    for (int i = 1; i + 1 < argc; ++i) if (argv[i] == key) return argv[i + 1];
    return def;
}

struct Row { std::string id, scenario, expected, actual, status; };

int main(int argc, char** argv) {
    std::string out_path = arg_value(argc, argv, "--out", "data/out/scenario_report.csv");
    std::filesystem::create_directories(std::filesystem::path(out_path).parent_path());
    std::vector<Row> rows;

    CostModel costs{5,5,5,5,10,5};
    RiskConfig risk;
    StrategyConfig scfg{costs.min_net_edge_paise, risk.max_order_qty, risk.max_book_age_us, 1};

    auto run_sim = [&](const std::string& scenario) {
        Simulator sim(costs, risk, scfg, ScenarioConfig::from_name(scenario));
        return sim.run_replay("data/sample_day.csv", "data/out/" + scenario, 1);
    };

    auto s01 = run_sim("clean");
    rows.push_back({"S01", "Clean arbitrage success", "FLAT", s01.final_state, s01.final_state == "FLAT" ? "PASS" : "FAIL"});

    CostModel high_costs{20,20,20,20,20,5};
    Simulator sim2(high_costs, risk, scfg, ScenarioConfig::from_name("clean"));
    auto s02 = sim2.run_replay("data/sample_day.csv", "data/out/edge_negative", 1);
    rows.push_back({"S02", "Gross spread positive but net edge negative", "no trades", std::to_string(s02.trades), s02.trades == 0 ? "PASS" : "FAIL"});

    auto s03 = run_sim("bse_buy_fills_nse_sell_reject");
    rows.push_back({"S03", "BSE buy fills, NSE sell rejects", "HEDGE then FLAT", s03.final_state, s03.final_state == "FLAT" ? "PASS" : "FAIL"});

    auto s04 = run_sim("nse_sell_fills_bse_buy_reject");
    rows.push_back({"S04", "NSE sell fills, BSE buy rejects", "HEDGE then FLAT", s04.final_state, s04.final_state == "FLAT" ? "PASS" : "FAIL"});

    auto s05 = run_sim("buy_partial_sell_zero");
    rows.push_back({"S05", "Buy partially fills, sell fails", "residual hedged", s05.final_state, s05.final_state == "FLAT" ? "PASS" : "FAIL"});

    auto s06 = run_sim("both_legs_partial");
    rows.push_back({"S06", "Both legs partially fill unequally", "residual hedged", s06.final_state, s06.final_state == "FLAT" ? "PASS" : "FAIL"});

    PositionBook pb;
    RiskEngine re(risk, pb);
    OrderIntent oi{"P", 1, Venue::NSE, Side::Buy, OrderType::Limit, TimeInForce::IOC, OrderRole::AlphaBuy, 110080, 10};
    BestQuote stale{Venue::NSE,1,110050,100,110080,100,999999};
    auto rd_stale = re.check_order(oi, stale, now_ns());
    rows.push_back({"S07", "Book stale on one venue", "STALE_BOOK", rd_stale.rule, rd_stale.rule == "STALE_BOOK" ? "PASS" : "FAIL"});

    SequenceTracker st;
    st.on_packet(Venue::NSE, 1, 1);
    auto gap = st.on_packet(Venue::NSE, 1, 3);
    rows.push_back({"S08", "Sequence gap in feed", "GAP", to_cstr(gap), gap == SequenceStatus::Gap ? "PASS" : "FAIL"});

    SequenceTracker st2;
    st2.on_packet(Venue::NSE, 1, 1);
    auto dup = st2.on_packet(Venue::NSE, 1, 1);
    rows.push_back({"S09", "Duplicate packet", "DUPLICATE", to_cstr(dup), dup == SequenceStatus::Duplicate ? "PASS" : "FAIL"});

    auto s10 = run_sim("gateway_disconnect_nse");
    rows.push_back({"S10", "Gateway disconnect", "risk/error state", s10.final_state, (s10.final_state == "ERROR_RECOVERY" || s10.final_state == "KILL_SWITCH") ? "PASS" : "FAIL"});

    auto s11 = run_sim("delayed_ack");
    rows.push_back({"S11", "ACK delayed", "no crash", s11.final_state, (s11.final_state == "FLAT" || s11.final_state == "") ? "PASS" : "FAIL"});

    Reconciler rec;
    OrderManager oms(pb);
    Fill orphan{"P", "UNKNOWN_ORDER", 1, Venue::NSE, Side::Buy, 1, 1, now_ns()};
    auto orphan_evt = rec.on_external_fill(orphan, oms);
    rows.push_back({"S12", "Unknown fill arrives", "ORPHAN_FILL", orphan_evt ? orphan_evt->rule_name : "NONE", orphan_evt && orphan_evt->rule_name == "ORPHAN_FILL" ? "PASS" : "FAIL"});

    RiskConfig risk_rate = risk;
    risk_rate.max_orders_per_sec = 1;
    PositionBook pb_rate;
    RiskEngine re_rate(risk_rate, pb_rate);
    BestQuote goodq{Venue::NSE,1,110050,100,110080,100,0};
    auto r1 = re_rate.check_order(oi, goodq, 1000000000ULL);
    auto r2 = re_rate.check_order(oi, goodq, 1000000001ULL);
    rows.push_back({"S13", "Order-rate limit breached", "ORDER_RATE", r2.rule, r1.approved && r2.rule == "ORDER_RATE" ? "PASS" : "FAIL"});

    PositionBook pb_cap;
    pb_cap.set_position(1, risk.max_position_qty);
    RiskEngine re_cap(risk, pb_cap);
    auto cap_decision = re_cap.check_order(oi, goodq, now_ns());
    rows.push_back({"S14", "Inventory cap breached", "MAX_POSITION", cap_decision.rule, cap_decision.rule == "MAX_POSITION" ? "PASS" : "FAIL"});

    PairTrade p;
    p.parent_id = "P"; p.instrument_id = 1; p.intended_sell_venue = Venue::NSE; p.bought_qty = 100; p.bought_notional_paise = 110000 * 100; p.state = PairState::Hedging;
    RiskConfig tight = risk; tight.max_hedge_loss_paise = 1;
    HedgePolicy hp(tight);
    BestQuote bad_sell{Venue::NSE,1,100000,100,100010,100,0};
    BestQuote bad_sell_bse{Venue::BSE,1,100000,100,100010,100,0};
    auto hd = hp.next_order(p, bad_sell, bad_sell_bse);
    rows.push_back({"S15", "Hedge price exceeds max loss", "MAX_HEDGE_LOSS", hd.reason, hd.kill && hd.reason == "MAX_HEDGE_LOSS" ? "PASS" : "FAIL"});

    bool eod = EodGuard::should_flatten(1000, 1500, 600, 10);
    rows.push_back({"S16", "End-of-day residual", "flatten", eod ? "flatten" : "hold", eod ? "PASS" : "FAIL"});

    CostModel bad_costs{};
    std::string cost_reason;
    bool cost_ok = bad_costs.validate(&cost_reason);
    rows.push_back({"S17", "Cost config wrong/missing", "fail safe", cost_ok ? "valid" : "invalid", !cost_ok ? "PASS" : "FAIL"});

    bool skew = ClockSkewGuard::severe_skew(1000, 5000000, 100000);
    rows.push_back({"S18", "Clock skew", "severe", skew ? "severe" : "ok", skew ? "PASS" : "FAIL"});

    OrderBook crossed(Venue::NSE, 1);
    crossed.add(1, Side::Buy, 100, 10);
    crossed.add(2, Side::Sell, 99, 10);
    rows.push_back({"S19", "Crossed/locked books", "crossed", crossed.crossed() ? "crossed" : "normal", crossed.crossed() ? "PASS" : "FAIL"});

    PositionBook pb_kill;
    RiskEngine re_kill(risk, pb_kill);
    re_kill.kill_switch().disable("manual");
    auto kill_decision = re_kill.check_order(oi, goodq, now_ns());
    rows.push_back({"S20", "Kill switch pressed", "KILL_SWITCH", kill_decision.rule, kill_decision.rule == "KILL_SWITCH" ? "PASS" : "FAIL"});

    auto esc = [](const std::string& x) {
        std::string y;
        bool quote = false;
        for (char c : x) {
            if (c == '"') { y += "\"\""; quote = true; }
            else { if (c == ',' || c == '\n') quote = true; y += c; }
        }
        return quote ? std::string("\"") + y + "\"" : y;
    };
    std::ofstream out(out_path);
    out << "id,scenario,expected,actual,status\n";
    for (const auto& r : rows) {
        out << esc(r.id) << ',' << esc(r.scenario) << ',' << esc(r.expected) << ',' << esc(r.actual) << ',' << esc(r.status) << '\n';
    }
    std::cout << "wrote " << out_path << " with " << rows.size() << " scenarios\n";
    return 0;
}
