# Queue Pipeline Benchmark Report

This benchmark compares two ways of moving events through the hot-path pipeline:

1. `normal_mutex_condition_variable_queue` - bounded queue protected by mutex and condition variables.
2. `spsc_lock_free_ring_buffer` - bounded single-producer/single-consumer ring buffers using acquire/release memory ordering.

## Benchmark setup

- Events: 500000
- Pipeline links: 1
- Normal queue capacity: 8192
- SPSC usable capacity: 8191
- Latency measured from producer creation timestamp to final sink consumption.
- CPU usage is process CPU time divided by wall time, so multi-threaded runs can exceed 100%.

## Results

| Metric | Normal queue | SPSC ring buffer |
|---|---:|---:|
| Events/sec | 2411072.28 | 4070715.49 |
| Average latency ns | 2670589.69 | 1668742.08 |
| p50 latency ns | 2774905 | 1804070 |
| p99 latency ns | 4204884 | 2127548 |
| p99.9 latency ns | 4495790 | 2216867 |
| Max latency ns | 4533852 | 2257115 |
| CPU usage % | 192.89 | 179.11 |
| Average queue occupancy | 6512.14 | 6937.16 |
| Max queue occupancy | 8192 | 8191 |
| Full/backpressure events | 206 | 8522 |
| Dropped events | 0 | 0 |

## How to read this

The SPSC design is expected to remove mutex and condition-variable overhead in one-to-one pipeline handoffs. It may use more CPU because hot-path consumers spin instead of sleeping; that tradeoff is common in low-latency systems. The important metrics to watch are p99/p99.9 latency, throughput, and whether backpressure occurs.
