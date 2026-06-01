#pragma once

#include <fstream>
#include <set>
#include <string>
#include "book/book_manager.hpp"
#include "hedge/hedge_policy.hpp"
#include "md/market_data_handler.hpp"
#include "metrics/latency_recorder.hpp"
#include "oms/order_manager.hpp"
#include "risk/risk_engine.hpp"
#include "sim/matching_engine.hpp"
#include "strategy/arbitrage_strategy.hpp"

namespace hft {

struct SimulatorResult {
    int opportunities{};
    int trades{};
    int hedges{};
    int risk_blocks{};
    std::string final_state;
};

class Simulator {
public:
    Simulator(CostModel costs, RiskConfig risk, StrategyConfig strategy, ScenarioConfig scenario);
    SimulatorResult run_replay(const std::string& replay_path, const std::string& out_dir, int max_trades = 1);

private:
    void open_logs(const std::string& out_dir);
    void close_logs();
    void log_opportunity(const StrategyDecision& d, const std::string& parent_id = "");
    void log_order(const Order& o);
    void log_fill(const Fill& f);
    void log_risk(uint64_t ts, const std::string& severity, const std::string& rule, uint32_t instrument_id, const std::string& details);
    void log_pair(const PairTrade& p);
    void log_all_orders_and_new_fills(OrderManager& oms);
    void execute_opportunity(const ArbOpportunity& opp, MatchingEngine& gateway, RiskEngine& risk_engine, OrderManager& oms, uint64_t now_ns, SimulatorResult& result);
    void hedge_until_flat(PairTrade& pair, MatchingEngine& gateway, RiskEngine& risk_engine, OrderManager& oms, uint64_t now_ns, SimulatorResult& result);

    CostModel costs_;
    RiskConfig risk_cfg_;
    StrategyConfig strategy_cfg_;
    ScenarioConfig scenario_;
    BookManager books_;
    std::ofstream opportunities_;
    std::ofstream orders_;
    std::ofstream fills_;
    std::ofstream risks_;
    std::ofstream pairs_;
    std::set<std::string> logged_order_ids_;
};

} // namespace hft
