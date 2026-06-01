#include "concurrency/bounded_blocking_queue.hpp"
#include "concurrency/spsc_ring_buffer.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <numeric>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#if defined(__linux__) || defined(__APPLE__)
#include <sys/resource.h>
#endif

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86)
#include <immintrin.h>
#endif

using hft::concurrency::BoundedBlockingQueue;
using hft::concurrency::SpscRingBuffer;

namespace {

constexpr std::size_t kSpscRawCapacity = 8192;
constexpr std::size_t kQueuesInPipeline = 1;
constexpr std::size_t kWorkerStages = kQueuesInPipeline - 1;

struct PipelineEvent {
    uint64_t seq{0};
    uint64_t created_ns{0};
    uint64_t payload{0};
    bool stop{false};
};

struct QueueStats {
    std::atomic<uint64_t> full_events{0};
    std::atomic<uint64_t> dropped_events{0};
    std::atomic<uint64_t> occupancy_samples{0};
    std::atomic<uint64_t> occupancy_sum{0};
    std::atomic<uint64_t> max_occupancy{0};

    void observe(std::size_t occupancy) {
        occupancy_samples.fetch_add(1, std::memory_order_relaxed);
        occupancy_sum.fetch_add(static_cast<uint64_t>(occupancy), std::memory_order_relaxed);

        auto current = max_occupancy.load(std::memory_order_relaxed);
        while (occupancy > current &&
               !max_occupancy.compare_exchange_weak(current, occupancy, std::memory_order_relaxed)) {
        }
    }
};

struct BenchmarkResult {
    std::string approach;
    uint64_t events_processed{0};
    double wall_seconds{0.0};
    double events_per_second{0.0};
    double average_latency_ns{0.0};
    uint64_t p50_latency_ns{0};
    uint64_t p99_latency_ns{0};
    uint64_t p999_latency_ns{0};
    uint64_t max_latency_ns{0};
    double cpu_usage_percent{0.0};
    double average_queue_occupancy{0.0};
    uint64_t max_queue_occupancy{0};
    uint64_t full_events{0};
    uint64_t dropped_events{0};
};

uint64_t now_ns() {
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

double process_cpu_seconds() {
#if defined(__linux__) || defined(__APPLE__)
    rusage usage{};
    if (getrusage(RUSAGE_SELF, &usage) != 0) {
        return 0.0;
    }
    const double user = static_cast<double>(usage.ru_utime.tv_sec) +
                        static_cast<double>(usage.ru_utime.tv_usec) / 1'000'000.0;
    const double sys = static_cast<double>(usage.ru_stime.tv_sec) +
                       static_cast<double>(usage.ru_stime.tv_usec) / 1'000'000.0;
    return user + sys;
#else
    return 0.0;
#endif
}

void cpu_relax(uint64_t spin_count) {
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86)
    _mm_pause();
#else
    (void)spin_count;
#endif
    if ((spin_count & 0x3fULL) == 0) {
        std::this_thread::yield();
    }
}

uint64_t do_stage_work(uint64_t value, std::size_t stage_id) {
    // Small deterministic CPU work to keep the compiler from optimizing the
    // pipeline into a pure queue benchmark while still keeping queue overhead visible.
    value ^= (0x9e3779b97f4a7c15ULL + stage_id + (value << 6U) + (value >> 2U));
    value *= 0xbf58476d1ce4e5b9ULL;
    value ^= value >> 31U;
    return value;
}

uint64_t percentile(std::vector<uint64_t>& values, double p) {
    if (values.empty()) {
        return 0;
    }
    const double rank = (p / 100.0) * static_cast<double>(values.size() - 1);
    const auto index = static_cast<std::size_t>(std::llround(rank));
    std::nth_element(values.begin(), values.begin() + static_cast<std::ptrdiff_t>(index), values.end());
    return values[index];
}

BenchmarkResult finalize_result(const std::string& approach,
                                uint64_t events,
                                double wall_seconds,
                                double cpu_delta_seconds,
                                const std::vector<uint64_t>& latency_ns,
                                const std::array<QueueStats, kQueuesInPipeline>& stats) {
    BenchmarkResult r;
    r.approach = approach;
    r.events_processed = events;
    r.wall_seconds = wall_seconds;
    r.events_per_second = wall_seconds > 0.0 ? static_cast<double>(events) / wall_seconds : 0.0;

    if (!latency_ns.empty()) {
        const uint64_t sum = std::accumulate(latency_ns.begin(), latency_ns.end(), uint64_t{0});
        r.average_latency_ns = static_cast<double>(sum) / static_cast<double>(latency_ns.size());
        std::vector<uint64_t> tmp = latency_ns;
        r.p50_latency_ns = percentile(tmp, 50.0);
        tmp = latency_ns;
        r.p99_latency_ns = percentile(tmp, 99.0);
        tmp = latency_ns;
        r.p999_latency_ns = percentile(tmp, 99.9);
        r.max_latency_ns = *std::max_element(latency_ns.begin(), latency_ns.end());
    }

    r.cpu_usage_percent = wall_seconds > 0.0 ? (cpu_delta_seconds / wall_seconds) * 100.0 : 0.0;

    uint64_t occupancy_sum = 0;
    uint64_t occupancy_samples = 0;
    uint64_t max_occupancy = 0;
    uint64_t full_events = 0;
    uint64_t dropped_events = 0;

    for (const auto& q : stats) {
        occupancy_sum += q.occupancy_sum.load(std::memory_order_relaxed);
        occupancy_samples += q.occupancy_samples.load(std::memory_order_relaxed);
        max_occupancy = std::max(max_occupancy, q.max_occupancy.load(std::memory_order_relaxed));
        full_events += q.full_events.load(std::memory_order_relaxed);
        dropped_events += q.dropped_events.load(std::memory_order_relaxed);
    }

    r.average_queue_occupancy = occupancy_samples > 0
                                    ? static_cast<double>(occupancy_sum) / static_cast<double>(occupancy_samples)
                                    : 0.0;
    r.max_queue_occupancy = max_occupancy;
    r.full_events = full_events;
    r.dropped_events = dropped_events;
    return r;
}

BenchmarkResult run_normal_queue_pipeline(uint64_t events, std::size_t capacity) {
    using Queue = BoundedBlockingQueue<PipelineEvent>;

    std::array<std::unique_ptr<Queue>, kQueuesInPipeline> queues;
    for (auto& q : queues) {
        q = std::make_unique<Queue>(capacity);
    }

    std::array<QueueStats, kQueuesInPipeline> stats;
    std::vector<uint64_t> latencies;
    latencies.reserve(static_cast<std::size_t>(events));

    const double cpu_before = process_cpu_seconds();
    const uint64_t wall_start = now_ns();

    auto push_wait = [&](std::size_t index, PipelineEvent&& event) {
        bool had_to_wait = false;
        queues[index]->push_wait(std::move(event), &had_to_wait);
        if (had_to_wait) {
            stats[index].full_events.fetch_add(1, std::memory_order_relaxed);
        }
        stats[index].observe(queues[index]->size_approx());
    };

    auto pop_wait = [&](std::size_t index, PipelineEvent& event) {
        queues[index]->pop_wait(event);
        stats[index].observe(queues[index]->size_approx());
    };

    std::thread producer([&] {
        for (uint64_t i = 0; i < events; ++i) {
            PipelineEvent event{i, now_ns(), i * 17U, false};
            push_wait(0, std::move(event));
        }
        push_wait(0, PipelineEvent{events, now_ns(), 0, true});
    });

    std::array<std::thread, kWorkerStages> workers;
    for (std::size_t stage = 0; stage < kWorkerStages; ++stage) {
        workers[stage] = std::thread([&, stage] {
            PipelineEvent event;
            for (;;) {
                pop_wait(stage, event);
                if (event.stop) {
                    push_wait(stage + 1, std::move(event));
                    break;
                }
                event.payload = do_stage_work(event.payload, stage + 1);
                push_wait(stage + 1, std::move(event));
            }
        });
    }

    std::thread sink([&] {
        PipelineEvent event;
        for (;;) {
            pop_wait(kQueuesInPipeline - 1, event);
            if (event.stop) {
                break;
            }
            event.payload = do_stage_work(event.payload, kQueuesInPipeline);
            latencies.push_back(now_ns() - event.created_ns);
        }
    });

    producer.join();
    for (auto& t : workers) {
        t.join();
    }
    sink.join();

    const uint64_t wall_end = now_ns();
    const double cpu_after = process_cpu_seconds();
    const double wall_seconds = static_cast<double>(wall_end - wall_start) / 1'000'000'000.0;

    return finalize_result("normal_mutex_condition_variable_queue", events, wall_seconds,
                           cpu_after - cpu_before, latencies, stats);
}

template <std::size_t Capacity>
BenchmarkResult run_spsc_pipeline(uint64_t events) {
    using Queue = SpscRingBuffer<PipelineEvent, Capacity>;

    std::array<std::unique_ptr<Queue>, kQueuesInPipeline> queues;
    for (auto& q : queues) {
        q = std::make_unique<Queue>();
    }

    std::array<QueueStats, kQueuesInPipeline> stats;
    std::vector<uint64_t> latencies;
    latencies.reserve(static_cast<std::size_t>(events));

    const double cpu_before = process_cpu_seconds();
    const uint64_t wall_start = now_ns();

    auto push_spin = [&](std::size_t index, PipelineEvent&& event) {
        uint64_t spins = 0;
        while (!queues[index]->try_push(std::move(event))) {
            stats[index].full_events.fetch_add(1, std::memory_order_relaxed);
            stats[index].observe(queues[index]->size_approx());
            cpu_relax(++spins);
        }
        stats[index].observe(queues[index]->size_approx());
    };

    auto pop_spin = [&](std::size_t index, PipelineEvent& event) {
        uint64_t spins = 0;
        while (!queues[index]->try_pop(event)) {
            cpu_relax(++spins);
        }
        stats[index].observe(queues[index]->size_approx());
    };

    std::thread producer([&] {
        for (uint64_t i = 0; i < events; ++i) {
            PipelineEvent event{i, now_ns(), i * 17U, false};
            push_spin(0, std::move(event));
        }
        push_spin(0, PipelineEvent{events, now_ns(), 0, true});
    });

    std::array<std::thread, kWorkerStages> workers;
    for (std::size_t stage = 0; stage < kWorkerStages; ++stage) {
        workers[stage] = std::thread([&, stage] {
            PipelineEvent event;
            for (;;) {
                pop_spin(stage, event);
                if (event.stop) {
                    push_spin(stage + 1, std::move(event));
                    break;
                }
                event.payload = do_stage_work(event.payload, stage + 1);
                push_spin(stage + 1, std::move(event));
            }
        });
    }

    std::thread sink([&] {
        PipelineEvent event;
        for (;;) {
            pop_spin(kQueuesInPipeline - 1, event);
            if (event.stop) {
                break;
            }
            event.payload = do_stage_work(event.payload, kQueuesInPipeline);
            latencies.push_back(now_ns() - event.created_ns);
        }
    });

    producer.join();
    for (auto& t : workers) {
        t.join();
    }
    sink.join();

    const uint64_t wall_end = now_ns();
    const double cpu_after = process_cpu_seconds();
    const double wall_seconds = static_cast<double>(wall_end - wall_start) / 1'000'000'000.0;

    return finalize_result("spsc_lock_free_ring_buffer", events, wall_seconds,
                           cpu_after - cpu_before, latencies, stats);
}

std::string csv_escape(const std::string& text) {
    std::string out;
    out.reserve(text.size() + 2);
    out.push_back('"');
    for (char c : text) {
        if (c == '"') {
            out += "\"\"";
        } else {
            out.push_back(c);
        }
    }
    out.push_back('"');
    return out;
}

void write_raw_csv(const std::filesystem::path& file, const std::vector<BenchmarkResult>& results) {
    std::ofstream out(file);
    out << "approach,events_processed,events_processed_per_second,average_queue_latency_ns,"
           "p50_latency_ns,p99_latency_ns,p99_9_latency_ns,max_latency_ns,cpu_usage_percent,"
           "average_queue_occupancy,max_queue_occupancy,full_events,dropped_events\n";
    out << std::fixed << std::setprecision(2);
    for (const auto& r : results) {
        out << csv_escape(r.approach) << ','
            << r.events_processed << ','
            << r.events_per_second << ','
            << r.average_latency_ns << ','
            << r.p50_latency_ns << ','
            << r.p99_latency_ns << ','
            << r.p999_latency_ns << ','
            << r.max_latency_ns << ','
            << r.cpu_usage_percent << ','
            << r.average_queue_occupancy << ','
            << r.max_queue_occupancy << ','
            << r.full_events << ','
            << r.dropped_events << '\n';
    }
}

void write_comparison_csv(const std::filesystem::path& file,
                          const BenchmarkResult& normal,
                          const BenchmarkResult& spsc) {
    struct Row {
        std::string metric;
        double normal_value;
        double spsc_value;
        bool lower_is_better;
        std::string unit;
    };

    const std::vector<Row> rows{
        {"events_processed_per_second", normal.events_per_second, spsc.events_per_second, false, "events/sec"},
        {"average_queue_latency", normal.average_latency_ns, spsc.average_latency_ns, true, "ns"},
        {"p50_latency", static_cast<double>(normal.p50_latency_ns), static_cast<double>(spsc.p50_latency_ns), true, "ns"},
        {"p99_latency", static_cast<double>(normal.p99_latency_ns), static_cast<double>(spsc.p99_latency_ns), true, "ns"},
        {"p99_9_latency", static_cast<double>(normal.p999_latency_ns), static_cast<double>(spsc.p999_latency_ns), true, "ns"},
        {"max_latency", static_cast<double>(normal.max_latency_ns), static_cast<double>(spsc.max_latency_ns), true, "ns"},
        {"cpu_usage", normal.cpu_usage_percent, spsc.cpu_usage_percent, true, "percent_process_cpu_over_wall"},
        {"average_queue_occupancy", normal.average_queue_occupancy, spsc.average_queue_occupancy, true, "events"},
        {"max_queue_occupancy", static_cast<double>(normal.max_queue_occupancy), static_cast<double>(spsc.max_queue_occupancy), true, "events"},
        {"full_events", static_cast<double>(normal.full_events), static_cast<double>(spsc.full_events), true, "count"},
        {"dropped_events", static_cast<double>(normal.dropped_events), static_cast<double>(spsc.dropped_events), true, "count"},
    };

    std::ofstream out(file);
    out << "metric,normal_queue,spsc_ring_buffer,delta_spsc_minus_normal,pct_change_spsc_vs_normal,unit,better\n";
    out << std::fixed << std::setprecision(2);
    for (const auto& row : rows) {
        const double delta = row.spsc_value - row.normal_value;
        const double pct = std::abs(row.normal_value) > 1e-12 ? (delta / row.normal_value) * 100.0 : 0.0;
        const bool spsc_better = row.lower_is_better ? (row.spsc_value < row.normal_value)
                                                     : (row.spsc_value > row.normal_value);
        out << row.metric << ','
            << row.normal_value << ','
            << row.spsc_value << ','
            << delta << ','
            << pct << ','
            << row.unit << ','
            << (spsc_better ? "spsc" : "normal_or_tie") << '\n';
    }
}

void write_markdown_report(const std::filesystem::path& file,
                           uint64_t events,
                           std::size_t normal_capacity,
                           std::size_t spsc_usable_capacity,
                           const BenchmarkResult& normal,
                           const BenchmarkResult& spsc) {
    std::ofstream out(file);
    out << "# Queue Pipeline Benchmark Report\n\n";
    out << "This benchmark compares two ways of moving events through the hot-path pipeline:\n\n";
    out << "1. `normal_mutex_condition_variable_queue` - bounded queue protected by mutex and condition variables.\n";
    out << "2. `spsc_lock_free_ring_buffer` - bounded single-producer/single-consumer ring buffers using acquire/release memory ordering.\n\n";
    out << "## Benchmark setup\n\n";
    out << "- Events: " << events << "\n";
    out << "- Pipeline links: " << kQueuesInPipeline << "\n";
    out << "- Normal queue capacity: " << normal_capacity << "\n";
    out << "- SPSC usable capacity: " << spsc_usable_capacity << "\n";
    out << "- Latency measured from producer creation timestamp to final sink consumption.\n";
    out << "- CPU usage is process CPU time divided by wall time, so multi-threaded runs can exceed 100%.\n\n";

    out << "## Results\n\n";
    out << "| Metric | Normal queue | SPSC ring buffer |\n";
    out << "|---|---:|---:|\n";
    out << std::fixed << std::setprecision(2);
    out << "| Events/sec | " << normal.events_per_second << " | " << spsc.events_per_second << " |\n";
    out << "| Average latency ns | " << normal.average_latency_ns << " | " << spsc.average_latency_ns << " |\n";
    out << "| p50 latency ns | " << normal.p50_latency_ns << " | " << spsc.p50_latency_ns << " |\n";
    out << "| p99 latency ns | " << normal.p99_latency_ns << " | " << spsc.p99_latency_ns << " |\n";
    out << "| p99.9 latency ns | " << normal.p999_latency_ns << " | " << spsc.p999_latency_ns << " |\n";
    out << "| Max latency ns | " << normal.max_latency_ns << " | " << spsc.max_latency_ns << " |\n";
    out << "| CPU usage % | " << normal.cpu_usage_percent << " | " << spsc.cpu_usage_percent << " |\n";
    out << "| Average queue occupancy | " << normal.average_queue_occupancy << " | " << spsc.average_queue_occupancy << " |\n";
    out << "| Max queue occupancy | " << normal.max_queue_occupancy << " | " << spsc.max_queue_occupancy << " |\n";
    out << "| Full/backpressure events | " << normal.full_events << " | " << spsc.full_events << " |\n";
    out << "| Dropped events | " << normal.dropped_events << " | " << spsc.dropped_events << " |\n\n";

    out << "## How to read this\n\n";
    out << "The SPSC design is expected to remove mutex and condition-variable overhead in one-to-one pipeline handoffs. "
           "It may use more CPU because hot-path consumers spin instead of sleeping; that tradeoff is common in low-latency systems. "
           "The important metrics to watch are p99/p99.9 latency, throughput, and whether backpressure occurs.\n";
}

uint64_t parse_u64_arg(int argc, char** argv, const std::string& name, uint64_t default_value) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (argv[i] == name) {
            return static_cast<uint64_t>(std::stoull(argv[i + 1]));
        }
    }
    return default_value;
}

std::string parse_string_arg(int argc, char** argv, const std::string& name, const std::string& default_value) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (argv[i] == name) {
            return argv[i + 1];
        }
    }
    return default_value;
}

void print_result(const BenchmarkResult& r) {
    std::cout << std::fixed << std::setprecision(2)
              << r.approach
              << " events=" << r.events_processed
              << " eps=" << r.events_per_second
              << " avg_ns=" << r.average_latency_ns
              << " p50_ns=" << r.p50_latency_ns
              << " p99_ns=" << r.p99_latency_ns
              << " p99_9_ns=" << r.p999_latency_ns
              << " max_ns=" << r.max_latency_ns
              << " cpu_pct=" << r.cpu_usage_percent
              << " avg_occ=" << r.average_queue_occupancy
              << " max_occ=" << r.max_queue_occupancy
              << " full_events=" << r.full_events
              << " dropped=" << r.dropped_events
              << '\n';
}

} // namespace

int main(int argc, char** argv) {
    const uint64_t events = parse_u64_arg(argc, argv, "--events", 500'000);
    const std::size_t normal_capacity = static_cast<std::size_t>(parse_u64_arg(argc, argv, "--normal-capacity", 8192));
    const std::filesystem::path out_dir = parse_string_arg(argc, argv, "--out", "data/out/queue_benchmark");

    std::filesystem::create_directories(out_dir);

    std::cout << "Running normal mutex/condition-variable pipeline...\n";
    auto normal = run_normal_queue_pipeline(events, normal_capacity);
    print_result(normal);

    std::cout << "Running SPSC ring-buffer pipeline...\n";
    auto spsc = run_spsc_pipeline<kSpscRawCapacity>(events);
    print_result(spsc);

    const auto raw_csv = out_dir / "queue_pipeline_benchmark.csv";
    const auto comparison_csv = out_dir / "queue_pipeline_comparison.csv";
    const auto report_md = out_dir / "queue_pipeline_benchmark_report.md";

    write_raw_csv(raw_csv, {normal, spsc});
    write_comparison_csv(comparison_csv, normal, spsc);
    write_markdown_report(report_md, events, normal_capacity,
                          SpscRingBuffer<PipelineEvent, kSpscRawCapacity>::usable_capacity(), normal, spsc);

    std::cout << "Wrote " << raw_csv << '\n';
    std::cout << "Wrote " << comparison_csv << '\n';
    std::cout << "Wrote " << report_md << '\n';
    return 0;
}
