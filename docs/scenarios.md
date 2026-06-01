# Scenario Playbooks

The scenario suite maps to the Excel playbook tab.

| ID | Scenario | Practical handling in code |
|---|---|---|
| S01 | Clean arbitrage success | Both IOC legs fill; `PairTrade` becomes `Flat`. |
| S02 | Gross spread positive but net edge negative | `ArbitrageStrategy` rejects because `net_edge_paise < min_net_edge_paise`. |
| S03 | BSE buy fills, NSE sell rejects | `PairTrade` residual > 0; `HedgePolicy` sends IOC SELL. |
| S04 | NSE sell fills, BSE buy rejects | `PairTrade` residual < 0; `HedgePolicy` sends IOC BUY. |
| S05 | Buy partially fills, sell fully fills requested smaller qty | Residual is based on actual fills, never intended qty. |
| S06 | Both legs partially fill unequally | Hedge only the remaining residual. |
| S07 | Book stale on one venue | `RiskEngine` returns `STALE_BOOK`. |
| S08 | Sequence gap in feed | `SequenceTracker` returns `Gap`; feed marked unsafe. |
| S09 | Duplicate packet | `SequenceTracker` returns `Duplicate`; packet ignored. |
| S10 | Gateway disconnect | Matching gateway rejects/blocks venue. |
| S11 | ACK delayed | OMS accepts fill before ACK and remains consistent. |
| S12 | Unknown fill arrives | `Reconciler` raises `ORPHAN_FILL`. |
| S13 | Order-rate limit breached | `RateLimiter` blocks new order. |
| S14 | Inventory cap breached | `RiskEngine` blocks with `MAX_POSITION`. |
| S15 | Hedge price exceeds max loss | `HedgePolicy` returns kill/error decision. |
| S16 | End-of-day residual | `EodGuard` requests flattening. |
| S17 | Cost config wrong/missing | `CostModel::validate()` fails safe. |
| S18 | Clock skew | `ClockSkewGuard` blocks severe timestamp mismatch. |
| S19 | Crossed/locked books | Book invariant flags data sanity issue. |
| S20 | Kill switch pressed | `RiskEngine` blocks with `KILL_SWITCH`. |
