#!/usr/bin/env bash
set -euo pipefail

EVENTS="${1:-500000}"
OUT_DIR="${2:-data/out/queue_benchmark}"

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/bench_queue_pipeline --events "$EVENTS" --out "$OUT_DIR"

echo ""
echo "Queue benchmark files written to $OUT_DIR"
echo "- $OUT_DIR/queue_pipeline_benchmark.csv"
echo "- $OUT_DIR/queue_pipeline_comparison.csv"
echo "- $OUT_DIR/queue_pipeline_benchmark_report.md"
