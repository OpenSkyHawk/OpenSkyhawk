#!/usr/bin/env bash
# Weekly refresh of InvenTree parts from LCSC (price / stock / datasheet / params).
# Uses inventree-part-import's own --update-recursive on our category tree.
# IPN-safe (the tool never touches IPN). Part names follow last-imported MPN — cosmetic, ignore.
#
# Schedule (macOS launchd example — weekly, Mon 03:00):
#   copy com.openskyhawk.inventree-weekly.plist -> ~/Library/LaunchAgents/ and `launchctl load` it,
# or cron:  0 3 * * 1  /path/to/tools/inventree-part-import/weekly-update.sh
set -uo pipefail

IPI="${IPI:-$HOME/ipi-venv/bin/python}"
LOG="${LOG:-$HOME/.local/state/inventree-weekly.log}"
mkdir -p "$(dirname "$LOG")"

{
  echo "===== inventree weekly update $(date) ====="
  # Electronics covers Passives/Semiconductors/Connectors/Crystals/Modules (all LCSC parts).
  # Controls (Alibaba toggles etc.) have no LCSC supplier — nothing to refresh, skip.
  "$IPI" -m inventree_part_import --update-recursive Electronics -i false
  echo "exit=$? at $(date)"
} >> "$LOG" 2>&1
