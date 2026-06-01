#!/usr/bin/env python3
import csv
import sys
from pathlib import Path


def main() -> int:
    in_path = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("data/out/scenario_report.csv")
    out_path = Path(sys.argv[2]) if len(sys.argv) > 2 else Path("data/out/scenario_report.md")
    rows = list(csv.DictReader(in_path.open()))
    out_path.parent.mkdir(parents=True, exist_ok=True)
    with out_path.open("w") as f:
        f.write("# Scenario Report\n\n")
        f.write("| ID | Scenario | Expected | Actual | Status |\n")
        f.write("|---|---|---|---|---|\n")
        for r in rows:
            f.write(f"| {r['id']} | {r['scenario']} | {r['expected']} | {r['actual']} | {r['status']} |\n")
    print(f"wrote {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
