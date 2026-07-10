#!/usr/bin/env bash
# Cost-refresh ROUTINE (run manually, on demand — NOT an OS-scheduled job).
# Re-pulls current LCSC price / stock / datasheet / params for every part in a category
# via inventree-part-import's own --update-recursive. IPN-safe (the tool never touches IPN).
# Part names follow last-imported MPN — cosmetic, ignore.
#
# Run it periodically (e.g. before placing an order, or ~monthly) to keep per-panel cost current:
#   ./refresh-costs.sh
set -uo pipefail

IPI="${IPI:-$HOME/ipi-venv/bin/python}"
LOG="${LOG:-$HOME/.local/state/inventree-refresh.log}"
mkdir -p "$(dirname "$LOG")"

echo "===== inventree cost refresh $(date) ====="
# Electronics covers Passives/Semiconductors/Connectors/Crystals/Modules (all LCSC parts).
# Controls (Alibaba toggles etc.) have no LCSC supplier — nothing to refresh, skip.
"$IPI" -m inventree_part_import --update-recursive Electronics -i false | tee -a "$LOG"
echo "exit=${PIPESTATUS[0]} at $(date)" | tee -a "$LOG"
