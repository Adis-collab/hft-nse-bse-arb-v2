#pragma once

#include "book/book_manager.hpp"
#include "oms/gateway.hpp"
#include "sim/scenario.hpp"

namespace hft {

class MatchingEngine : public IOrderGateway {
public:
    MatchingEngine(BookManager& books, ScenarioConfig scenario) : books_(books), scenario_(std::move(scenario)) {}
    std::vector<ExecutionReport> send(const Order& order) override;
    ScenarioConfig& scenario() { return scenario_; }

private:
    BookManager& books_;
    ScenarioConfig scenario_;
};

} // namespace hft
