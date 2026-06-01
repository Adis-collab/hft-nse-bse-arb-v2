# Version 2: SPSC Ring Buffer Pipeline Upgrade

Version 2 adds a production-style SPSC ring-buffer implementation and a benchmark that compares it against a normal mutex/condition-variable queue pipeline.

The goal is not to claim that every part of the simulator is a production trading system. The goal is to make the hot-path communication model more realistic for a low-latency trading architecture and to make the performance tradeoffs measurable.

## Why this upgrade exists

The project pipeline is naturally staged:

```text
Market data replay
    -> normalization
    -> order book reconstruction
    -> strategy / signal generation
    -> risk checks
    -> OMS / execution
```

A beginner implementation often connects these stages with normal shared queues such as:

```cpp
std::queue<Event> queue;
std::mutex mutex;
std::condition_variable cv;
```

That works, but it introduces locking, condition-variable wakeups, context switching, and unpredictable p99/p99.9 latency.

For a hot HFT-style pipeline, many adjacent stages are naturally one-to-one handoffs. A dedicated SPSC ring buffer is a better fit there.

## What SPSC means

SPSC means **single producer, single consumer**.

Only one thread pushes into a queue. Only one thread pops from that queue.

That restriction is powerful because the queue no longer needs a mutex. The producer owns the write cursor and the consumer owns the read cursor.

## Traditional queue approach

```text
Producer thread
    -> lock mutex
    -> push into queue
    -> unlock mutex
    -> notify condition variable

Consumer thread
    -> wait / wake
    -> lock mutex
    -> pop from queue
    -> unlock mutex
    -> process event
```

### Strengths

- Easy to understand.
- Good for background jobs and non-hot paths.
- Works with multiple producers/consumers if written carefully.

### Weaknesses in low latency paths

- Lock contention.
- Condition-variable sleep/wake cost.
- Context switches.
- Unstable tail latency.
- More OS scheduler involvement.

## SPSC ring-buffer approach

```text
Producer thread
    -> check if ring has space
    -> construct event in ring slot
    -> publish head cursor

Consumer thread
    -> check if ring has data
    -> move event from ring slot
    -> publish tail cursor
```

### Strengths

- No mutex in the hot handoff.
- No condition variable in the hot handoff.
- Fixed-size memory.
- No heap allocation in push/pop.
- Better cache locality.
- Lower overhead for one-to-one pipeline links.

### Tradeoffs

- It only works when there is exactly one producer and one consumer.
- It is bounded; full queues must be handled explicitly.
- Busy-waiting can use more CPU than a sleeping condition-variable queue.
- It requires correct acquire/release memory ordering.

## Implemented files

```text
src/concurrency/spsc_ring_buffer.hpp       Production-style SPSC ring buffer
src/concurrency/bounded_blocking_queue.hpp Reference mutex + condition-variable queue
bench/bench_queue_pipeline.cpp             Normal queue vs SPSC benchmark
scripts/run_queue_benchmark.sh             One-command benchmark runner
docs/v2_spsc_pipeline_upgrade.md           This design note
docs/assets/spsc_vs_mutex_pipeline.png     Architecture visual
```

## SPSC design details

The implementation in `src/concurrency/spsc_ring_buffer.hpp` uses:

- power-of-two capacity
- one-slot-empty full/empty distinction
- raw storage with placement-new
- move-only type support
- `std::construct_at` / `std::destroy_at`
- acquire/release memory ordering
- 64-byte alignment for cursors
- no heap allocation during push/pop

The producer publishes data like this:

```cpp
std::construct_at(ptr(head), std::forward<Args>(args)...);
head_.value.store(next, std::memory_order_release);
```

The consumer observes data like this:

```cpp
if (tail == head_.value.load(std::memory_order_acquire)) {
    return false;
}
```

Simple meaning:

```text
release = I have finished writing; now I publish it.
acquire = I see the publish; now I can safely read it.
```

## Benchmark command

```bash
bash scripts/run_queue_benchmark.sh
```

or manually:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/bench_queue_pipeline --events 500000 --out data/out/queue_benchmark
```

## Benchmark scope

The benchmark intentionally measures a representative one-to-one handoff rather than the full trading simulator. This isolates the communication primitive itself:

```text
Producer thread -> queue implementation -> Consumer thread
```

That is the same handoff pattern used between hot-path stages such as replay -> normalization, book -> strategy, and strategy -> risk.


Default benchmark settings in Version 2:

```text
Events: 500,000
Normal queue capacity: 8,192
SPSC raw capacity: 8,192
SPSC usable capacity: 8,191
```

## Benchmark outputs

```text
data/out/queue_benchmark/queue_pipeline_benchmark.csv
data/out/queue_benchmark/queue_pipeline_comparison.csv
data/out/queue_benchmark/queue_pipeline_benchmark_report.md
```

The benchmark reports:

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

## How to interpret results

SPSC should usually improve throughput and reduce synchronization overhead. It can also reduce tail latency when the pipeline is not overloaded.

However, if the SPSC producer is much faster than downstream stages, it may fill the ring buffer and generate many backpressure events. That is expected and useful information: it tells you where the pipeline bottleneck is.

Mutex queues may show lower CPU usage because waiting threads sleep. SPSC hot paths may show higher CPU usage because low-latency systems often spin briefly rather than sleep.

The correct interview-level explanation is:

> I replaced generic locked queues with bounded SPSC ring buffers on one-to-one hot-path links. This removes mutex and condition-variable overhead, makes ownership clearer, improves throughput, and gives measurable p50/p99/p99.9 latency behaviour. I still keep normal queues acceptable for cold paths like logging and reporting.
