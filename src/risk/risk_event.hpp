#pragma once

#include <cstdint>
#include <string>

namespace hft {

struct RiskEvent {
    uint64_t ts_ns{};
    std::string severity;
    std::string rule_name;
    uint32_t instrument_id{};
    std::string details_json;
};

struct RiskDecision {
    bool approved{false};
    std::string rule;
    std::string details;

    static RiskDecision approve() { return {true, "APPROVED", ""}; }
    static RiskDecision block(std::string rule, std::string details) { return {false, std::move(rule), std::move(details)}; }
};

} // namespace hft
