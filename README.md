# NSE/BSE Equity Arbitrage HFT Portfolio Project - Version 2

Educational, exchange-shaped C++20 simulator for NSE-vs-BSE cash-equity cross-venue arbitrage.

This repository is **not a live trading system** and does not connect to NSE/BSE production gateways. It is a portfolio-grade research and simulation codebase that mirrors the components a fresher HFT / trading-systems candidate should understand.

## Version 2 headline upgrade

Version 2 adds a **production-style SPSC ring-buffer hot-path upgrade** and a benchmark that compares it with a conventional mutex/condition-variable queue pipeline.

The new benchmark reports:

- events processed per second
- average queue latency
- p50 latency
- p99 latency
- p99.9 latency
- max latency
- CPU usage
- average/max queue occupancy
- full/backpressure events
- dropped events

## Implemented system components

- market data replay and normalization
- sequence gap and duplicate detection
- deterministic order book reconstruction
- cross-venue arbitrage signal generation
- configurable costs, buffers, and risk checks
- OMS state machine with ACK/fill/reject/expire handling
- legging-risk recovery and hedge policy
- matching simulator with reject, partial-fill, disconnect, and delayed-ACK scenarios
- latency measurement and CSV reports
- scenario playbooks aligned to the development guide
- **new in v2:** production-style SPSC ring buffer implementation
- **new in v2:** normal queue vs SPSC pipeline benchmark
- **new in v2:** queue benchmark CSV + Markdown reporting

## Quick start

```bash
sudo apt-get update
sudo apt-get install -y cmake ninja-build g++ python3

cd hft-nse-bse-arb-v2
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

## Run the main arbitrage demo

```bash
./build/hft_demo \
  --instruments config/instruments.csv \
  --costs config/costs.yml \
  --risk config/risk.yml \
  --replay data/sample_day.csv \
  --out data/out \
  --scenario clean

python3 tools/report_pnl.py data/out/fills.csv data/out/pnl_summary.csv
```

Or run:

```bash
bash scripts/run_demo.sh
```

## Run all scenario playbooks

```bash
bash scripts/run_all_scenarios.sh
```

This generates:

```text
data/out/scenario_report.csv
data/out/scenario_report.md
```

## Run the Version 2 SPSC benchmark

```bash
bash scripts/run_queue_benchmark.sh
```

or manually:

```bash
./build/bench_queue_pipeline --events 500000 --out data/out/queue_benchmark
```

Generated files:

```text
data/out/queue_benchmark/queue_pipeline_benchmark.csv
data/out/queue_benchmark/queue_pipeline_comparison.csv
data/out/queue_benchmark/queue_pipeline_benchmark_report.md
```

The included sample benchmark output is under:

```text
data/example_out/queue_benchmark/
```

## Why the SPSC upgrade matters

The simulator pipeline is naturally staged:

```text
Market Data Replay
    -> Normalization
    -> Order Book Reconstruction
    -> Strategy / Signal Generation
    -> Risk Checks
    -> OMS / Execution
```

A normal queue often uses:

```cpp
std::queue<Event> queue;
std::mutex mutex;
std::condition_variable cv;
```

That is fine for general software, but hot trading paths care about p99 and p99.9 latency. Locks, condition-variable wakeups, and context switches can introduce unpredictable tail latency.

The SPSC model uses a fixed-size one-to-one ring buffer:

```text
Producer thread -> SPSC Ring Buffer -> Consumer thread
```

It removes mutex/condition-variable overhead from one-to-one handoffs and makes the ownership model clearer:

- producer owns the write cursor
- consumer owns the read cursor
- acquire/release memory ordering publishes data safely
- no heap allocation occurs during push/pop

Detailed design note:

```text
docs/v2_spsc_pipeline_upgrade.md
```

Architecture visual:

```text
docs/assets/spsc_vs_mutex_pipeline.png
```

## Important files to study

```text
src/concurrency/spsc_ring_buffer.hpp       SPSC ring buffer implementation
src/concurrency/bounded_blocking_queue.hpp Normal mutex/CV reference queue
bench/bench_queue_pipeline.cpp             Queue benchmark implementation
src/book/order_book.*                      Deterministic book reconstruction
src/strategy/arbitrage_strategy.*          Cross-venue net-edge logic
src/risk/risk_engine.*                     Pre-trade controls
src/hedge/hedge_policy.*                   Legging-risk recovery
src/sim/matching_engine.*                  IOC matching simulation
apps/scenario_suite.cpp                    Failure scenario runner
```

## What to show in interviews

1. `src/concurrency/spsc_ring_buffer.hpp` - fixed-size, lock-free one-to-one queue with acquire/release memory ordering.
2. `bench/bench_queue_pipeline.cpp` - measured comparison against a normal mutex/CV pipeline.
3. `src/book/order_book.*` - deterministic book reconstruction.
4. `src/strategy/arbitrage_strategy.*` - net-edge calculation using bid/ask, not LTP.
5. `src/risk/risk_engine.*` - pre-trade controls with rule IDs.
6. `src/hedge/hedge_policy.*` - handling BSE-buy-filled/NSE-sell-failed and the reverse.
7. `src/sim/matching_engine.*` - venue-aware IOC limit matching, partial fills, rejects, delayed ACK.
8. `tests/test_main.cpp` - unit, integration, and failure scenario tests.

## Project boundary

The code intentionally avoids direct exchange connectivity. Before any real trading, validate current NSE/BSE/SEBI rules, broker access, exchange conformance requirements, costs, taxes, short-selling/settlement constraints, co-location, and clearing arrangements.

## Included example outputs

A previously generated sample run is included under:

```text
data/example_out/
```

Fresh runs write to:

```text
data/out/
```
