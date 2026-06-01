#include "sim/simulator.hpp"
#include <iostream>
#include <string>
#include <unordered_map>

using namespace hft;

static std::string arg_value(int argc, char** argv, const std::string& key, const std::string& def = "") {
    for (int i = 1; i + 1 < argc; ++i) {
        if (argv[i] == key) return argv[i + 1];
    }
    return def;
}

int main(int argc, char** argv) {
    try {
        std::string costs_path = arg_value(argc, argv, "--costs", "config/costs.yml");
        std::string risk_path = arg_value(argc, argv, "--risk", "config/risk.yml");
        std::string replay_path = arg_value(argc, argv, "--replay", "data/sample_day.csv");
        std::string out_dir = arg_value(argc, argv, "--out", "data/out");
        std::string scenario_name = arg_value(argc, argv, "--scenario", "clean");
        (void)arg_value(argc, argv, "--instruments", "config/instruments.csv");

        CostModel costs = CostModel::load(costs_path);
        std::string reason;
        if (!costs.validate(&reason)) {
            std::cerr << "Cost config invalid: " << reason << "\n";
            return 2;
        }
        RiskConfig risk = RiskConfig::load(risk_path);
        StrategyConfig scfg;
        scfg.min_net_edge_paise = costs.min_net_edge_paise;
        scfg.max_qty = risk.max_order_qty;
        scfg.max_book_age_us = risk.max_book_age_us;
        ScenarioConfig scenario = ScenarioConfig::from_name(scenario_name);

        Simulator sim(costs, risk, scfg, scenario);
        SimulatorResult result = sim.run_replay(replay_path, out_dir, 1);
        std::cout << "scenario=" << scenario_name
                  << " opportunities=" << result.opportunities
                  << " trades=" << result.trades
                  << " hedges=" << result.hedges
                  << " risk_blocks=" << result.risk_blocks
                  << " final_state=" << result.final_state << "\n";
        std::cout << "outputs written to " << out_dir << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
