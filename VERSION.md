# Version 2 Release Notes

## Release: v2-spsc-pipeline-benchmark

This release upgrades the project with a dedicated low-latency concurrency module and a reproducible benchmark comparing a normal mutex/condition-variable queue with a production-style SPSC ring buffer.

## Added

- `src/concurrency/spsc_ring_buffer.hpp`
  - bounded SPSC ring buffer
  - raw storage + placement-new
  - move-only type support
  - acquire/release memory ordering
  - cache-line aligned cursors
  - no heap allocation in push/pop

- `src/concurrency/bounded_blocking_queue.hpp`
  - reference bounded queue using mutex + condition variable

- `bench/bench_queue_pipeline.cpp`
  - normal queue vs SPSC hot-path handoff benchmark
  - reports throughput, average latency, p50, p99, p99.9, max latency, CPU usage, occupancy, full/backpressure events, dropped events

- `scripts/run_queue_benchmark.sh`
  - one-command benchmark execution

- `docs/v2_spsc_pipeline_upgrade.md`
  - detailed explanation of why SPSC is used, where it fits, and how to interpret results

- `docs/assets/spsc_vs_mutex_pipeline.png`
  - architecture visual comparing both approaches

## Verification performed

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
bash scripts/run_demo.sh
bash scripts/run_all_scenarios.sh
./build/bench_queue_pipeline --out data/example_out/queue_benchmark
```

## Boundary

This remains an educational/research simulator and does not connect to NSE/BSE production gateways.
