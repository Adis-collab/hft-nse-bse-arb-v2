#!/usr/bin/env bash
set -euo pipefail

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure

rm -rf data/out
mkdir -p data/out

./build/hft_demo \
  --instruments config/instruments.csv \
  --costs config/costs.yml \
  --risk config/risk.yml \
  --replay data/sample_day.csv \
  --out data/out \
  --scenario clean

python3 tools/report_pnl.py data/out/fills.csv data/out/pnl_summary.csv
printf '\nDemo complete. Inspect data/out/*.csv\n'
