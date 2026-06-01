# Verification

The repository was built and tested in the provided environment with:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
bash scripts/run_demo.sh
bash scripts/run_all_scenarios.sh
```

Observed status:

- C++ build completed successfully.
- `hft_tests` passed.
- `hft_demo` produced `opportunities.csv`, `orders.csv`, `fills.csv`, `risk_events.csv`, `pair_trades.csv`, `latency.csv`, and `pnl_summary.csv`.
- `scenario_suite` generated 20 scenario rows and all were PASS after validation.

This verification is for the educational simulator only. It is not certification for live exchange trading.
