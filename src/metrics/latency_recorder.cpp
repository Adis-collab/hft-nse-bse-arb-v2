#include "metrics/latency_recorder.hpp"
#include <algorithm>
#include <sstream>

namespace hft {

LatencyRecorder& LatencyRecorder::instance() {
    static LatencyRecorder r;
    return r;
}

void LatencyRecorder::record(const std::string& stage, uint64_t ns) {
    std::lock_guard<std::mutex> lock(mu_);
    data_[stage].push_back(ns);
}

std::string LatencyRecorder::summary_csv() const {
    std::lock_guard<std::mutex> lock(mu_);
    std::ostringstream out;
    out << "stage,count,p50_ns,p99_ns,p999_ns,max_ns\n";
    for (auto [stage, vals] : data_) {
        if (vals.empty()) continue;
        std::sort(vals.begin(), vals.end());
        auto pct = [&](double p) {
            size_t idx = static_cast<size_t>(p * static_cast<double>(vals.size() - 1));
            if (idx >= vals.size()) idx = vals.size() - 1;
            return vals[idx];
        };
        out << stage << ',' << vals.size() << ',' << pct(0.50) << ',' << pct(0.99) << ',' << pct(0.999) << ',' << vals.back() << '\n';
    }
    return out.str();
}

void LatencyRecorder::clear() {
    std::lock_guard<std::mutex> lock(mu_);
    data_.clear();
}

StageTimer::~StageTimer() {
    auto end = Clock::now();
    auto ns = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(end - start_).count());
    LatencyRecorder::instance().record(stage_, ns);
}

} // namespace hft
