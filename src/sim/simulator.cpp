#include "sim/simulator.hpp"
#include "common/csv.hpp"
#include "common/ids.hpp"
#include <filesystem>
#include <iostream>
#include <sstream>

namespace hft {

Simulator::Simulator(CostModel costs, RiskConfig risk, StrategyConfig strategy, ScenarioConfig scenario)
    : costs_(costs), risk_cfg_(risk), strategy_cfg_(strategy), scenario_(std::move(scenario)) {}

void Simulator::open_logs(const std::string& out_dir) {
    logged_order_ids_.clear();
    std::filesystem::create_directories(out_dir);
    opportunities_.open(out_dir + "/opportunities.csv");
    orders_.open(out_dir + "/orders.csv");
    fills_.open(out_dir + "/fills.csv");
    risks_.open(out_dir + "/risk_events.csv");
    pairs_.open(out_dir + "/pair_trades.csv");
    opportunities_ << "parent_arb_id,ts_decision_ns,instrument_id,buy_venue,sell_venue,buy_price_paise,sell_price_paise,gross_spread_paise,net_edge_paise,qty,decision_reason\n";
    orders_ << "parent_arb_id,client_order_id,instrument_id,venue,side,role,price_paise,qty,filled_qty,state,ts_sent_ns,ts_ack_ns,ts_done_ns,err_code\n";
    fills_ << "parent_arb_id,client_order_id,instrument_id,venue,side,fill_px_paise,fill_qty,ts_fill_ns\n";
    risks_ << "ts_ns,severity,rule_name,instrument_id,details\n";
    pairs_ << "parent_arb_id,instrument_id,state,bought_qty,sold_qty,residual,hedge_attempts,last_error\n";
}

void Simulator::close_logs() {
    opportunities_.close();
    orders_.close();
    fills_.close();
    risks_.close();
    pairs_.close();
}

void Simulator::log_opportunity(const StrategyDecision& d, const std::string& parent_id) {
    if (d.should_trade) {
        const auto& o = d.opportunity;
        opportunities_ << o.parent_arb_id << ',' << o.ts_decision_ns << ',' << o.instrument_id << ',' << to_string(o.buy_venue) << ','
                       << to_string(o.sell_venue) << ',' << o.buy_price_paise << ',' << o.sell_price_paise << ',' << o.gross_spread_paise << ','
                       << o.net_edge_paise << ',' << o.qty << ',' << o.reason << '\n';
    } else {
        opportunities_ << parent_id << ",0,0,UNKNOWN,UNKNOWN,0,0," << d.best_gross_spread_paise << ',' << d.best_net_edge_paise << ",0," << d.reason << '\n';
    }
}

void Simulator::log_order(const Order& o) {
    orders_ << o.intent.parent_arb_id << ',' << o.client_order_id << ',' << o.intent.instrument_id << ',' << to_string(o.intent.venue) << ','
            << to_string(o.intent.side) << ',' << to_string(o.intent.role) << ',' << o.intent.price_paise << ',' << o.intent.qty << ','
            << o.filled_qty << ',' << to_string(o.state) << ',' << o.ts_sent_ns << ',' << o.ts_ack_ns << ',' << o.ts_done_ns << ',' << o.err_code << '\n';
}

void Simulator::log_fill(const Fill& f) {
    fills_ << f.parent_arb_id << ',' << f.client_order_id << ',' << f.instrument_id << ',' << to_string(f.venue) << ',' << to_string(f.side) << ','
           << f.fill_px_paise << ',' << f.fill_qty << ',' << f.ts_fill_ns << '\n';
}

void Simulator::log_risk(uint64_t ts, const std::string& severity, const std::string& rule, uint32_t instrument_id, const std::string& details) {
    risks_ << ts << ',' << severity << ',' << rule << ',' << instrument_id << ',' << details << '\n';
}

void Simulator::log_pair(const PairTrade& p) {
    pairs_ << p.parent_id << ',' << p.instrument_id << ',' << to_string(p.state) << ',' << p.bought_qty << ',' << p.sold_qty << ','
           << p.residual() << ',' << p.hedge_attempts << ',' << p.last_error << '\n';
}

void Simulator::log_all_orders_and_new_fills(OrderManager& oms) {
    for (const auto& [id, o] : oms.orders()) {
        if (logged_order_ids_.insert(id).second) {
            log_order(o);
        }
    }
    for (const auto& f : oms.take_new_fills()) log_fill(f);
}

SimulatorResult Simulator::run_replay(const std::string& replay_path, const std::string& out_dir, int max_trades) {
    open_logs(out_dir);
    LatencyRecorder::instance().clear();

    MarketDataHandler md(replay_path);
    ArbitrageStrategy strategy(strategy_cfg_);
    PositionBook positions;
    RiskEngine risk_engine(risk_cfg_, positions);
    if (scenario_.manual_kill) risk_engine.kill_switch().disable("MANUAL_KILL_SCENARIO");
    OrderManager oms(positions);
    MatchingEngine gateway(books_, scenario_);
    IdGenerator ids("SIM");
    SimulatorResult result;

    while (auto item = md.next()) {
        uint64_t now = 0;
        {
            StageTimer t("md_to_book");
            if (item->event) {
                now = item->event->ts_ingest_ns;
                books_.on_event(*item->event);
                if (item->sequence_status == SequenceStatus::Gap) {
                    log_risk(now, "HIGH", "SEQUENCE_GAP", item->event->instrument_id, item->warning);
                    result.risk_blocks++;
                }
            } else {
                continue;
            }
        }

        uint32_t instrument_id = item->event->instrument_id;
        auto nse_q = books_.quote(Venue::NSE, instrument_id, now);
        auto bse_q = books_.quote(Venue::BSE, instrument_id, now);
        if (!nse_q || !bse_q) continue;

        // Feed health can mark a venue unsafe after a gap.
        if (!md.health().is_healthy(Venue::NSE, instrument_id) || !md.health().is_healthy(Venue::BSE, instrument_id)) {
            log_risk(now, "HIGH", "UNHEALTHY_FEED", instrument_id, "feed marked unsafe");
            result.risk_blocks++;
            continue;
        }

        StrategyDecision d;
        {
            StageTimer t("strategy");
            d = strategy.evaluate(*nse_q, *bse_q, costs_, now);
        }
        if (!d.should_trade) {
            if (d.reason != "INVALID_QUOTE") log_opportunity(d);
            continue;
        }

        d.opportunity.parent_arb_id = ids.next_parent(d.opportunity.instrument_id);
        log_opportunity(d);
        result.opportunities++;
        if (result.trades >= max_trades) continue;
        execute_opportunity(d.opportunity, gateway, risk_engine, oms, now, result);
        result.trades++;
    }

    // Write latency summary.
    std::string latency_path = out_dir + "/latency.csv";
    ensure_parent_dir(latency_path);
    std::ofstream lat(latency_path);
    lat << LatencyRecorder::instance().summary_csv();

    close_logs();
    return result;
}

void Simulator::execute_opportunity(const ArbOpportunity& opp, MatchingEngine& gateway, RiskEngine& risk_engine, OrderManager& oms, uint64_t now_ns, SimulatorResult& result) {
    PairTrade pair = PairTrade::from_opportunity(opp);
    auto nse_q = books_.quote(Venue::NSE, opp.instrument_id, now_ns).value_or(BestQuote{});
    auto bse_q = books_.quote(Venue::BSE, opp.instrument_id, now_ns).value_or(BestQuote{});

    OrderIntent buy;
    buy.parent_arb_id = opp.parent_arb_id;
    buy.instrument_id = opp.instrument_id;
    buy.venue = opp.buy_venue;
    buy.side = Side::Buy;
    buy.order_type = OrderType::Limit;
    buy.tif = TimeInForce::IOC;
    buy.role = OrderRole::AlphaBuy;
    buy.price_paise = opp.buy_price_paise;
    buy.qty = opp.qty;

    OrderIntent sell = buy;
    sell.venue = opp.sell_venue;
    sell.side = Side::Sell;
    sell.role = OrderRole::AlphaSell;
    sell.price_paise = opp.sell_price_paise;

    auto quote_for = [&](const OrderIntent& o) -> BestQuote {
        return o.venue == Venue::NSE ? nse_q : bse_q;
    };

    auto send_one = [&](const OrderIntent& intent) {
        RiskDecision rd;
        {
            StageTimer t("risk");
            rd = risk_engine.check_order(intent, quote_for(intent), now_ns);
        }
        if (!rd.approved) {
            log_risk(now_ns, "HIGH", rd.rule, intent.instrument_id, rd.details);
            pair.mark_leg_failed(rd.rule, now_ns);
            result.risk_blocks++;
            return;
        }
        {
            StageTimer t("oms_gateway");
            Order& o = oms.submit(intent, gateway, now_ns);
            log_order(o);
            auto new_fills = oms.take_new_fills();
            for (const auto& f : new_fills) {
                log_fill(f);
                if (f.parent_arb_id == pair.parent_id) pair.apply_fill(f);
            }
            if (o.state == OrderState::Rejected || o.state == OrderState::Expired) pair.mark_leg_failed(o.err_code, o.ts_done_ns);
        }
    };

    if (scenario_.send_sell_first) {
        send_one(sell);
        send_one(buy);
    } else {
        send_one(buy);
        send_one(sell);
    }


    if (pair.residual() != 0) {
        pair.state = PairState::Hedging;
        hedge_until_flat(pair, gateway, risk_engine, oms, now_ns, result);
    }
    if (pair.residual() == 0) pair.state = PairState::Flat;
    log_pair(pair);
    result.final_state = to_string(pair.state);
}

void Simulator::hedge_until_flat(PairTrade& pair, MatchingEngine& gateway, RiskEngine& risk_engine, OrderManager& oms, uint64_t now_ns, SimulatorResult& result) {
    HedgePolicy hedge(risk_cfg_);
    while (pair.residual() != 0 && pair.hedge_attempts < risk_cfg_.max_hedge_attempts) {
        auto nse_q = books_.quote(Venue::NSE, pair.instrument_id, now_ns).value_or(BestQuote{});
        auto bse_q = books_.quote(Venue::BSE, pair.instrument_id, now_ns).value_or(BestQuote{});
        HedgeDecision hd = hedge.next_order(pair, nse_q, bse_q);
        if (hd.kill) {
            pair.state = PairState::KillSwitch;
            pair.last_error = hd.reason;
            risk_engine.kill_switch().disable(hd.reason);
            log_risk(now_ns, "CRITICAL", hd.reason, pair.instrument_id, "hedge policy kill");
            result.risk_blocks++;
            break;
        }
        if (!hd.should_send) {
            pair.state = PairState::ErrorRecovery;
            pair.last_error = hd.reason;
            log_risk(now_ns, "HIGH", hd.reason, pair.instrument_id, "no hedge order produced");
            result.risk_blocks++;
            break;
        }
        ++pair.hedge_attempts;
        RiskDecision rd = risk_engine.check_order(hd.intent, hd.intent.venue == Venue::NSE ? nse_q : bse_q, now_ns);
        if (!rd.approved) {
            pair.state = PairState::ErrorRecovery;
            pair.last_error = rd.rule;
            log_risk(now_ns, "HIGH", rd.rule, pair.instrument_id, rd.details);
            result.risk_blocks++;
            break;
        }
        Order& o = oms.submit(hd.intent, gateway, now_ns);
        log_order(o);
        auto new_fills = oms.take_new_fills();
        for (const auto& f : new_fills) {
            log_fill(f);
            if (f.parent_arb_id == pair.parent_id) {
                // PairTrade counts all fills, including hedges, to drive residual to zero.
                pair.apply_fill(f);
            }
        }
        result.hedges++;
    }
    if (pair.residual() == 0) pair.state = PairState::Flat;
    else if (pair.state == PairState::Hedging) pair.state = PairState::ErrorRecovery;
}

} // namespace hft
