# Codebase Index - Version 2

This repository is organized to match the Excel guide's components and tracker, with a Version 2 SPSC hot-path upgrade.

## Start here

| Need | File |
|---|---|
| Build and run | `README.md`, `scripts/run_demo.sh` |
| Run queue benchmark | `scripts/run_queue_benchmark.sh` |
| Version 2 SPSC design | `docs/v2_spsc_pipeline_upgrade.md` |
| Queue benchmark sample output | `data/example_out/queue_benchmark/` |
| Coding tracker | `docs/development_index.md` |
| System design | `docs/architecture.md` |
| Component-by-component guide | `docs/component_walkthrough.md` |
| Failure scenarios | `docs/scenarios.md`, `apps/scenario_suite.cpp` |
| Interview prep | `docs/interview_notes.md` |

## Main executables

| Executable | Purpose |
|---|---|
| `hft_demo` | Runs replay -> strategy -> risk -> OMS -> simulator -> CSV logs |
| `scenario_suite` | Runs scenario playbook checks |
| `bench_pipeline` | Measures basic book/strategy latency stages |
| `bench_queue_pipeline` | Compares normal mutex/CV queue pipeline vs SPSC ring-buffer pipeline |
| `hft_tests` | Runs custom C++ unit/integration tests |

## Version 2 files

| File | Purpose |
|---|---|
| `src/concurrency/spsc_ring_buffer.hpp` | Production-style bounded SPSC ring buffer |
| `src/concurrency/bounded_blocking_queue.hpp` | Conventional mutex + condition-variable bounded queue for comparison |
| `bench/bench_queue_pipeline.cpp` | Queue pipeline benchmark implementation |
| `scripts/run_queue_benchmark.sh` | One-command benchmark runner |
| `docs/v2_spsc_pipeline_upgrade.md` | Explanation of the SPSC upgrade, tradeoffs, and benchmark interpretation |
| `docs/assets/spsc_vs_mutex_pipeline.png` | Visual comparison of the two approaches |

## Core folders

| Folder | Component |
|---|---|
| `src/concurrency` | SPSC ring buffer and bounded blocking queue reference implementation |
| `src/md` | Market data replay, parser, sequence tracking, feed health |
| `src/book` | Order book and multi-venue book manager |
| `src/strategy` | Cost model and arbitrage signal generation |
| `src/risk` | Pre-trade risk, position book, kill switch, guards |
| `src/oms` | Order intents, states, fills, gateway interface, reconciliation |
| `src/hedge` | PairTrade state machine and residual hedge policy |
| `src/sim` | Matching engine, failure scenarios, replay simulator |
| `src/metrics` | Latency recorder and stage timer |
| `bench` | Performance benchmarks |
| `tests` | Unit and integration tests |
| `tools` | Python CSV reports |
