# Architecture

The simulator is split into a hot path and a cold path.

## Hot path

```text
Replay/packet source -> MarketDataHandler -> BookManager -> ArbitrageStrategy -> RiskEngine -> OrderManager -> MatchingEngine -> PairTrade/HedgePolicy
```

The hot path uses in-memory C++ objects. No database calls, blocking dashboards, or heavyweight logging are required for a decision.

## Cold path

CSV outputs and SQL schemas are provided for analytics:

- `opportunities.csv`
- `orders.csv`
- `fills.csv`
- `risk_events.csv`
- `latency.csv`
- `scenario_report.csv`

## Key design decisions

1. Use integer paise for money.
2. Use bid/ask, not last traded price.
3. Treat two-leg arbitrage as a paired trade, never independent orders.
4. Assume one leg can fail; hedge the residual from actual fills.
5. Log every block reason with a rule ID.
6. Start with deterministic replay before any live connectivity.

## Version 2 hot-path communication model

Version 2 introduces a dedicated SPSC ring-buffer implementation for one-to-one pipeline handoffs.

```text
Market Data Replay Thread
    -> SPSC<Raw/Book Event>
Normalization / Book Thread
    -> SPSC<BestQuote/Signal Input>
Strategy Thread
    -> SPSC<OrderIntent>
Risk / OMS Thread
```

The benchmark in `bench/bench_queue_pipeline.cpp` compares this style with a conventional mutex/condition-variable queue pipeline. The SPSC implementation is in `src/concurrency/spsc_ring_buffer.hpp`.

Visual reference:

```text
docs/assets/spsc_vs_mutex_pipeline.png
```
