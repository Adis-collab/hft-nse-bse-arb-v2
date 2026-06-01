# Development Index

Use this file as the codebase version of the Excel coding tracker. Mark each task as done as you implement or modify it.

| ID | Phase | Component | Deliverable | Primary files | Status |
|---|---|---|---|---|---|
| P0-01 | Foundation | Problem | README explains NSE/BSE arbitrage, assumptions, non-live scope | README.md | Done |
| P0-02 | Foundation | Instruments | Liquid sample universe | config/instruments.csv | Done |
| P0-03 | Foundation | Costs | Configurable fees/taxes/buffers | config/costs.yml, src/strategy/cost_model.* | Done |
| P0-04 | Foundation | Repo | Professional folders | src/, tests/, docs/, tools/, sql/ | Done |
| P0-05 | Foundation | Build | CMake project | CMakeLists.txt | Done |
| P0-06 | Foundation | CI | Build/test workflow | .github/workflows/ci.yml | Done |
| P1-01 | Market Data | Packet schema | RawPacket | src/md/packet.hpp | Done |
| P1-02 | Market Data | Replay reader | Deterministic replay | src/md/replay_reader.* | Done |
| P1-03 | Market Data | Sequence tracking | GAP/DUPLICATE/RESET detection | src/md/sequence_tracker.hpp | Done |
| P1-04 | Market Data | Normalizer/parser | CSV replay -> BookEvent | src/md/csv_parser.* | Done |
| P1-05 | Market Data | Feed health | Staleness/unsafe book | src/md/feed_health.hpp | Done |
| P1-06 | Market Data | Fixtures | Sample replay files | data/*.csv, tests/test_main.cpp | Done |
| P2-01 | Book Engine | Data structures | Price levels + order index | src/book/order_book.* | Done |
| P2-02 | Book Engine | Order index | order_id -> side/price/qty | src/book/order_book.* | Done |
| P2-03 | Book Engine | Best quote | bid/ask snapshot | src/book/best_quote.hpp | Done |
| P2-04 | Book Engine | Invariants | no negative qty, index checks | src/book/order_book.* | Done |
| P2-05 | Book Engine | Multi-venue | BookManager keyed by venue+instrument | src/book/book_manager.* | Done |
| P3-01 | Strategy | Spread math | both directions evaluated | src/strategy/arbitrage_strategy.* | Done |
| P3-02 | Strategy | Net edge | costs and buffers subtracted | src/strategy/cost_model.* | Done |
| P3-03 | Strategy | Depth check | qty bounded by visible depth | src/strategy/arbitrage_strategy.* | Done |
| P3-04 | Strategy | Opportunity object | auditable decision | src/strategy/opportunity.hpp | Done |
| P3-05 | Strategy | Signal tests | profitable/non-profitable/stale | tests/test_main.cpp | Done |
| P4-01 | Risk | Risk config | risk.yml and loader | config/risk.yml, src/risk/risk_config.* | Done |
| P4-02 | Risk | Pre-trade check | APPROVE/BLOCK with rule IDs | src/risk/risk_engine.* | Done |
| P4-03 | Risk | Inventory | PositionBook from fills | src/risk/position_book.* | Done |
| P4-04 | Risk | Stale guard | max_book_age_us | src/risk/risk_engine.* | Done |
| P4-05 | Risk | Kill switch | global trading disable | src/risk/kill_switch.hpp | Done |
| P5-01 | OMS | Order intent | plan before send | src/oms/order_intent.hpp | Done |
| P5-02 | OMS | Client IDs | unique IDs | src/common/ids.hpp, src/oms/order_manager.* | Done |
| P5-03 | OMS | State machine | ACK/fill/reject/expire | src/oms/order_manager.* | Done |
| P5-04 | OMS | Reconciliation | orphan fill detection | src/oms/reconciler.hpp | Done |
| P5-05 | OMS | Simulator gateway | exchange-shaped gateway interface | src/oms/gateway.hpp, src/sim/matching_engine.* | Done |
| P6-01 | Hedging | Pair state | residual = bought - sold | src/hedge/pair_trade.* | Done |
| P6-02 | Hedging | Long residual | sell to flatten | src/hedge/hedge_policy.* | Done |
| P6-03 | Hedging | Short residual | buy to flatten | src/hedge/hedge_policy.* | Done |
| P6-04 | Hedging | Loss cap | max hedge attempts/loss | src/hedge/hedge_policy.* | Done |
| P7-01 | Simulator | Replay loop | full pipeline | src/sim/simulator.* | Done |
| P7-02 | Simulator | Latency injection | configurable simulated latency | src/sim/scenario.* | Done |
| P7-03 | Simulator | Matching | IOC limit fill/partial/expire | src/sim/matching_engine.* | Done |
| P7-04 | Simulator | Scenario tests | 20 playbook scenarios | apps/scenario_suite.cpp | Done |
| P7-05 | Simulator | Reports | opportunities/orders/fills/risk/latency CSV | src/sim/simulator.* | Done |
| P8-01 | Performance | Stage timers | p50/p99 summaries | src/metrics/latency_recorder.* | Done |
| P8-02 | Performance | Benchmarks | pipeline benchmark | bench/bench_pipeline.cpp | Done |
| P8-03 | Performance | Tuning notes | Linux checklist | docs/performance.md | Done |
| P8-04 | Performance | Regression evidence | CI + benchmark CSV | .github/workflows/ci.yml | Done |
| P9-01 | Testing | Unit tests | component tests | tests/test_main.cpp | Done |
| P9-02 | Testing | Golden replay | sample_day determinism | tests/test_main.cpp | Done |
| P9-03 | Testing | Failure tests | reject/legging/gap/stale | tests/test_main.cpp, scenario_suite | Done |
| P9-04 | Testing | Sanitizers | ASAN/UBSAN CI job | .github/workflows/ci.yml | Done |
| P9-05 | Testing | Docs | architecture/scenario/whitepaper docs | docs/ | Done |
| P9-06 | Testing | Demo script | one-command run | scripts/run_demo.sh | Done |
| P9-07 | Testing | Portfolio prep | resume/interview notes | docs/interview_notes.md | Done |
| V2-01 | Concurrency | SPSC queue | Production-style bounded SPSC ring buffer | src/concurrency/spsc_ring_buffer.hpp | Done |
| V2-02 | Concurrency | Reference queue | Mutex + condition-variable bounded queue for comparison | src/concurrency/bounded_blocking_queue.hpp | Done |
| V2-03 | Performance | Queue benchmark | SPSC vs normal queue pipeline benchmark | bench/bench_queue_pipeline.cpp | Done |
| V2-04 | Performance | Benchmark reports | CSV + Markdown benchmark output | data/example_out/queue_benchmark/ | Done |
| V2-05 | Documentation | SPSC guide | Detailed SPSC upgrade explanation | docs/v2_spsc_pipeline_upgrade.md | Done |
| V2-06 | CI | Benchmark smoke test | Queue benchmark compiled and smoke-tested in CI | .github/workflows/ci.yml | Done |
