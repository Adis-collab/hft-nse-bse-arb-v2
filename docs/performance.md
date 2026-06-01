# Latency and Performance Notes

Beginner targets from the guide:

| Stage | Good portfolio target |
|---|---:|
| Packet receive + decode | 3-8 us p99 |
| Book update + normalization | 3-8 us p99 |
| Signal computation | 2-6 us p99 |
| Risk + serialization | 3-10 us p99 |
| Wire-to-decision total | 15-30 us p99 |
| Decision-to-send | 25-50 us p99 |

This codebase includes `StageTimer` and `LatencyRecorder`. Run `bench_pipeline` after building.

Later tuning checklist:

- pin hot threads to isolated cores
- keep NIC interrupts away from strategy cores
- disable deep power-saving states during benchmark runs
- compare p50, p99, and p99.9, not only average
- avoid heap allocation in the hot loop after correctness is proven
- keep dashboards, databases, and reports outside the hot path

## Version 2 queue benchmark

Version 2 adds a dedicated benchmark for the pipeline communication layer:

```bash
bash scripts/run_queue_benchmark.sh
```

This produces:

```text
data/out/queue_benchmark/queue_pipeline_benchmark.csv
data/out/queue_benchmark/queue_pipeline_comparison.csv
data/out/queue_benchmark/queue_pipeline_benchmark_report.md
```

The benchmark compares:

| Approach | Description |
|---|---|
| Normal queue | Bounded queue using `std::mutex` + `std::condition_variable` |
| SPSC ring buffer | Fixed-size single-producer/single-consumer ring buffer using acquire/release memory ordering |

Reported metrics:

- events processed per second
- average queue latency
- p50 latency
- p99 latency
- p99.9 latency
- max latency
- CPU usage
- average queue occupancy
- max queue occupancy
- full/backpressure events
- dropped events

Interpretation note: SPSC ring buffers remove mutex/condition-variable overhead and are a better fit for one-to-one hot-path handoffs. They can consume more CPU because hot-path threads spin briefly instead of sleeping. In low-latency systems this can be an acceptable tradeoff when it improves throughput and tail-latency stability.
