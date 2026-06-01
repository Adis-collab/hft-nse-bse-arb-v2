# Whitepaper Draft

## Problem

The same equity can show temporary executable price differences between NSE and BSE. A theoretical opportunity exists when the cheap venue ask is below the rich venue bid after costs and buffers.

## Why this is hard

The opportunity is not guaranteed profit. Quotes can change, order queues can disappear, one order can fill while the other rejects, and fees/taxes/buffers can remove edge.

## Approach

Build a deterministic replay and execution simulation platform:

1. normalize replayed market data into `BookEvent`
2. reconstruct per-venue order books
3. compute bid/ask cross-venue net edge
4. send exchange-shaped IOC limit orders through a simulator
5. apply strict pre-trade risk checks
6. recover from one-leg failures using a residual hedge manager
7. measure latency and produce scenario reports

## Implemented solution

See `README.md`, `docs/architecture.md`, and the source folders.
