#!/usr/bin/env python3
"""Validate NODE_ID assignments against the registry.

Production panel groups live under ``Firmware/Panels/<name>/`` and declare their
NODE_ID via ``build_flags = -DNODE_ID=N`` in ``platformio.ini``. This script enforces:

  * every production panel declares a NODE_ID,
  * each NODE_ID is in range 1-63 (0 is reserved for PanelBridge),
  * no two panels share a NODE_ID,
  * each NODE_ID is registered in ``Firmware/NODE_IDS.md`` with a matching name.

Templates, Tests, and Examples are intentionally excluded — they reuse NODE_IDs by
design and are not real boards on a cockpit bus.

Exit code 0 on success, 1 on any violation.
"""
from __future__ import annotations

import glob
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
REGISTRY = ROOT / "Firmware" / "NODE_IDS.md"
PANEL_INIS = ROOT / "Firmware" / "Panels"


def parse_registry() -> dict[int, str]:
    """Return {node_id: panel_group_name} from the registry markdown table."""
    rows: dict[int, str] = {}
    for line in REGISTRY.read_text(encoding="utf-8").splitlines():
        m = re.match(r"\|\s*(\d+)\s*\|\s*([^|]+?)\s*\|", line)
        if m:
            rows[int(m.group(1))] = m.group(2).strip()
    return rows


def parse_panels() -> list[tuple[str, int | None, pathlib.Path]]:
    """Return (dir_name, node_id, ini_path) for each production panel."""
    out = []
    for ini in sorted(glob.glob(str(PANEL_INIS / "*" / "platformio.ini"))):
        path = pathlib.Path(ini)
        m = re.search(r"-DNODE_ID=(\d+)", path.read_text(encoding="utf-8"))
        out.append((path.parent.name, int(m.group(1)) if m else None, path))
    return out


def main() -> int:
    errors: list[str] = []

    registry = parse_registry()
    if registry.get(0, "").lower() != "panelbridge":
        errors.append("Registry: NODE_ID 0 must be reserved for PanelBridge")

    seen: dict[int, str] = {}
    panels = parse_panels()
    for name, nid, ini in panels:
        rel = ini.relative_to(ROOT)
        if nid is None:
            errors.append(f"{rel}: no -DNODE_ID=N found in build_flags")
            continue
        if not 1 <= nid <= 63:
            errors.append(f"{rel}: NODE_ID={nid} out of range 1-63 (0 is reserved for PanelBridge)")
        if nid in seen:
            errors.append(f"NODE_ID {nid} is used by both '{seen[nid]}' and '{name}' — must be unique")
        seen[nid] = name
        if nid not in registry:
            errors.append(f"'{name}': NODE_ID {nid} is not in Firmware/NODE_IDS.md — register it in the same PR")
        elif registry[nid] != name:
            errors.append(
                f"NODE_ID {nid}: registry lists '{registry[nid]}' but the panel directory is '{name}' — names must match"
            )

    if errors:
        print("NODE_ID validation FAILED:")
        for e in errors:
            print(f"  - {e}")
        return 1

    print(f"NODE_ID validation OK — {len(panels)} production panel(s) checked against the registry.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
