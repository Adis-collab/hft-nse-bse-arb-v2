# Component Walkthrough

This guide explains each component theoretically and practically.

## 1. Market data handler

**Problem:** NSE and BSE style feeds are venue-specific. A strategy should not know native packet details.

**Approach:** Store a `RawPacket`, check sequence numbers, parse replay CSV into a normalized `BookEvent`, and update feed health.

**Files:**

- `src/md/packet.hpp`
- `src/md/replay_reader.*`
- `src/md/csv_parser.*`
- `src/md/sequence_tracker.hpp`
- `src/md/feed_health.hpp`
- `src/md/market_data_handler.*`

**Practical flow:**

```text
ReplayReader::next_packet()
  -> SequenceTracker::on_packet()
  -> CsvParser::parse()
  -> FeedHealth::on_event() or mark_unhealthy()
  -> BookEvent
```

## 2. Order book engine

**Problem:** Arbitrage must use executable bid/ask and quantity, not last traded price.

**Approach:** Keep price levels plus an order ID index. Apply ADD/MODIFY/CANCEL/TRADE events deterministically.

**Files:**

- `src/book/order_book.*`
- `src/book/book_manager.*`
- `src/book/best_quote.hpp`

**Practical flow:**

```text
BookEvent -> BookManager::on_event() -> OrderBook::apply() -> BestQuote
```

## 3. Strategy engine

**Problem:** A positive gross spread may disappear after fees, taxes, slippage, latency, and hedge-risk buffer.

**Approach:** Evaluate both directions:

```text
Direction A: buy BSE ask, sell NSE bid
Direction B: buy NSE ask, sell BSE bid
net_edge = gross_spread - all_costs_and_buffers
```

**Files:**

- `src/strategy/cost_model.*`
- `src/strategy/arbitrage_strategy.*`
- `src/strategy/opportunity.hpp`

## 4. Risk engine

**Problem:** A strategy bug can send oversized, stale, or runaway orders.

**Approach:** Every `OrderIntent` passes risk before OMS submission.

Checks include:

- kill switch
- valid quote
- stale book
- max quantity
- max notional
- max position
- price band
- order-rate throttle

**Files:**

- `src/risk/risk_engine.*`
- `src/risk/risk_config.*`
- `src/risk/position_book.*`
- `src/risk/rate_limiter.hpp`
- `src/risk/kill_switch.hpp`
- `src/risk/guards.hpp`

## 5. OMS and gateway

**Problem:** Orders are asynchronous. ACK does not mean fill. Fill may be partial. Rejects and expiries must be explicit.

**Approach:** Convert a risk-approved `OrderIntent` into an `Order`, send it through an `IOrderGateway`, then process execution reports.

**Files:**

- `src/oms/order_intent.hpp`
- `src/oms/order_state.hpp`
- `src/oms/gateway.hpp`
- `src/oms/order_manager.*`
- `src/oms/reconciler.hpp`

## 6. Legging and hedging

**Problem:** BSE buy can fill while NSE sell fails, or NSE sell can fill while BSE buy fails.

**Approach:** Use a paired-trade state machine.

```text
residual = bought_qty - sold_qty
residual > 0 => long, sell to flatten
residual < 0 => short, buy to flatten
```

**Files:**

- `src/hedge/pair_trade.*`
- `src/hedge/hedge_policy.*`

## 7. Simulator and matching engine

**Problem:** Live HFT deployment is not beginner-friendly because approvals, memberships, co-location, clearing, and audits are real constraints.

**Approach:** Run deterministic replay and exchange-shaped execution simulation.

**Files:**

- `src/sim/matching_engine.*`
- `src/sim/scenario.*`
- `src/sim/simulator.*`
- `apps/hft_demo.cpp`
- `apps/scenario_suite.cpp`

## 8. Latency and reporting

**Problem:** HFT claims must be measured, not guessed.

**Approach:** Use `StageTimer` around core stages and export latency CSV.

**Files:**

- `src/metrics/latency_recorder.*`
- `bench/bench_pipeline.cpp`
- `tools/report_pnl.py`
- `tools/scenario_report.py`
