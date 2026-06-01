# Interview Notes

## Why not guarantee both orders execute?

Exchange order books are dynamic. By the time an order reaches the venue, available liquidity may be gone or the gateway can reject/expire the order. The correct design assumes one leg can fill and the other can fail.

## Why IOC limit orders?

IOC attempts immediate execution and cancels the remainder. Limit price caps the worst acceptable price. This is safer to model than a market order.

## What is legging risk?

Legging risk is the risk that only one side of a paired trade executes, leaving long or short inventory. In this codebase, `PairTrade::residual()` equals `bought_qty - sold_qty`.

## Why integer paise?

Money calculations should be deterministic and avoid floating point rounding errors.

## What makes this HFT-like?

Order book reconstruction, low-latency C++ hot path, risk controls, OMS state machine, deterministic replay, latency measurements, and failure playbooks.

## Version 2 SPSC queue interview talking points

### Why replace a mutex queue with SPSC?

Because the hot path is a sequence of one-to-one handoffs. A mutex queue is general but pays for locking, condition-variable wakeups, and scheduler involvement. An SPSC ring buffer is narrower but faster for exactly one producer and one consumer.

### Why bounded?

Bounded memory gives predictable behaviour. In a trading system, hidden memory growth is dangerous. If the queue is full, that is a visible backpressure event.

### Why power-of-two capacity?

It allows fast wraparound:

```cpp
next = (index + 1) & (capacity - 1);
```

### Why acquire/release?

The producer constructs the event first, then publishes the head cursor with release. The consumer observes the head with acquire before reading the event. This prevents the consumer from seeing the cursor update before the data is visible.

### Why can SPSC still show full events?

Because if the producer outruns the consumer, the fixed ring fills. That is expected and useful: it exposes backpressure instead of hiding it behind unbounded allocation.

### Where should SPSC not be used?

Do not use SPSC when there are multiple producers or multiple consumers for the same queue. Use MPSC/MPMC or redesign the ownership model.
