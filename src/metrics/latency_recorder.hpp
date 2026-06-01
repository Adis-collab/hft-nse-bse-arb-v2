#pragma once

#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace hft {

class LatencyRecorder {
public:
    static LatencyRecorder& instance();
    void record(const std::string& stage, uint64_t ns);
    std::string summary_csv() const;
    void clear();

private:
    mutable std::mutex mu_;
    std::unordered_map<std::string, std::vector<uint64_t>> data_;
};

class StageTimer {
public:
    explicit StageTimer(std::string stage) : stage_(std::move(stage)), start_(Clock::now()) {}
    ~StageTimer();
private:
    using Clock = std::chrono::steady_clock;
    std::string stage_;
    Clock::time_point start_;
};

} // namespace hft
