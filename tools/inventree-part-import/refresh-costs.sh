#!/usr/bin/env bash
# Cost-refresh ROUTINE (run manually, on demand — NOT an OS-scheduled job).
#
# Re-pulls current price / stock / datasheet / params for every part InvenTree already
# holds a supplier SKU for, via inventree-part-import (a local CLI in ~/ipi-venv — NOT
# an InvenTree plugin; the server's LCSC/DigiKey plugins are barcode scanners and do
# not fetch pricing).
#
# Safe on existing data: existing records are matched by SKU
# (part_importer.py -> get_supplier_part(api, supplier, SKU)), so nothing is
# duplicated, and "IPN" appears nowhere in the package, so IPNs are never touched.
# Part names follow last-imported MPN — cosmetic, ignore.
#
# Why SKUs and not --update-recursive: that mode searches by part NAME, and neither
# supplier module resolves a name reliably. LCSC returns nothing for an MPN
# ("STM32F103C8T6" -> 0 results) but everything for a C-number. DigiKey raises
# KeyError 'ProductsCount' on any name it does not know — upstream
# 30350n/inventree-part-import#118 — and that error aborts the entire run, so the
# category sweep used to exit 0 having refreshed 1 part out of 75. Feeding it the SKUs
# InvenTree already stores makes every lookup an exact match and avoids both bugs.
#
# Each supplier is passed only its own SKUs, via -o, so neither module ever sees the
# other's part numbers.
#
# Run before placing an order, or ~monthly:
#   ./refresh-costs.sh
set -uo pipefail

IPI="${IPI:-$HOME/ipi-venv/bin/python}"
LOG="${LOG:-$HOME/.local/state/inventree-refresh.log}"
mkdir -p "$(dirname "$LOG")"

SKUDIR="$(mktemp -d -t inventree-refresh)"
trap 'rm -rf "$SKUDIR"' EXIT

echo "===== inventree cost refresh $(date) ====="

# Ask InvenTree which SKUs it holds, one file per configured supplier module.
# Parts marked inactive are skipped: a refresh sets active=True again (base.py ->
# get_part_data), which would quietly un-retire them.
"$IPI" - "$SKUDIR" <<'PY'
import sys
from pathlib import Path

from inventree.company import SupplierPart
from inventree.part import Part

from inventree_part_import.cli import setup_inventree_api

SUPPLIERS = {1: "lcsc", 39: "digikey"}  # InvenTree company pk -> part-import module

out = Path(sys.argv[1])
api = setup_inventree_api()
inactive = {part.pk for part in Part.list(api, limit=1000) if not part.active}
for pk, name in SUPPLIERS.items():
    skus = sorted(
        supplier_part.SKU
        for supplier_part in SupplierPart.list(api, supplier=pk, limit=1000)
        if supplier_part.part not in inactive
    )
    (out / f"{name}.txt").write_text("\n".join(skus) + "\n")
    print(f"{name}: {len(skus)} SKUs")
PY

status=0
for supplier in lcsc digikey; do
    list="$SKUDIR/$supplier.txt"
    if [ ! -s "$list" ]; then
        echo "-- $supplier: no SKUs, skipping" | tee -a "$LOG"
        continue
    fi
    echo "-- refreshing $supplier ($(wc -l < "$list" | tr -d ' ') SKUs)" | tee -a "$LOG"
    # Unquoted on purpose: one SKU per line, none contain whitespace.
    # shellcheck disable=SC2046
    "$IPI" -m inventree_part_import -o "$supplier" -i false $(cat "$list") | tee -a "$LOG"
    rc=${PIPESTATUS[0]}
    [ "$rc" -eq 0 ] || status="$rc"
done

echo "exit=$status at $(date)" | tee -a "$LOG"
exit "$status"
