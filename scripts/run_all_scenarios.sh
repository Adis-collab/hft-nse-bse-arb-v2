#!/usr/bin/env bash
set -euo pipefail

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build

mkdir -p data/out
./build/scenario_suite --out data/out/scenario_report.csv
python3 tools/scenario_report.py data/out/scenario_report.csv data/out/scenario_report.md
cat data/out/scenario_report.md
