#!/usr/bin/env python3
import csv
import sys
from pathlib import Path


def main() -> int:
    in_path = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("data/out/fills.csv")
    out_path = Path(sys.argv[2]) if len(sys.argv) > 2 else Path("data/out/pnl_summary.csv")
    if not in_path.exists():
        print(f"fills file not found: {in_path}")
        return 1

    by_parent = {}
    with in_path.open() as f:
        reader = csv.DictReader(f)
        for row in reader:
            parent = row["parent_arb_id"]
            side = row["side"]
            px = int(row["fill_px_paise"])
            qty = int(row["fill_qty"])
            cash = -px * qty if side == "BUY" else px * qty
            rec = by_parent.setdefault(parent, {"gross_cash_paise": 0, "shares_traded": 0, "buy_qty": 0, "sell_qty": 0})
            rec["gross_cash_paise"] += cash
            rec["shares_traded"] += qty
            if side == "BUY":
                rec["buy_qty"] += qty
            else:
                rec["sell_qty"] += qty

    out_path.parent.mkdir(parents=True, exist_ok=True)
    with out_path.open("w", newline="") as f:
        fields = ["parent_arb_id", "gross_cash_paise", "gross_cash_rupees", "shares_traded", "buy_qty", "sell_qty", "residual_qty"]
        writer = csv.DictWriter(f, fieldnames=fields)
        writer.writeheader()
        for parent, rec in sorted(by_parent.items()):
            writer.writerow({
                "parent_arb_id": parent,
                "gross_cash_paise": rec["gross_cash_paise"],
                "gross_cash_rupees": f"{rec['gross_cash_paise'] / 100:.2f}",
                "shares_traded": rec["shares_traded"],
                "buy_qty": rec["buy_qty"],
                "sell_qty": rec["sell_qty"],
                "residual_qty": rec["buy_qty"] - rec["sell_qty"],
            })
    print(f"wrote {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
