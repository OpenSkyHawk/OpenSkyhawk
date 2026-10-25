#!/usr/bin/env python3
"""
gen_a4ec.py — OpenSkyhawk A4EC header generator.

Parses the DCS-BIOS A-4E-C.jsonp control definitions and emits:
  Firmware/Libraries/A4EC/A4EC_CmdIds.h      — DCSIN_* compact CAN transport IDs
  Firmware/Libraries/A4EC/A4EC_OutputIds.h   — A_4E_C_* output address/mask constants
  Firmware/Libraries/A4EC/A4EC_InputMap.h    — DcsBiosInputEntry[] dispatch table
  Firmware/Libraries/A4EC/GENERATOR_GAPS.md  — skipped controls with reasons

Usage (from repo root):
  python tools/gen_a4ec/gen_a4ec.py [--source <path>] [--timestamp <value>] [--change-report <path>]

Source resolution (priority order):
  1. --source <path>           explicit override, any platform
  2. Platform auto-detect:
       Windows: %USERPROFILE%\\Saved Games\\DCS\\Scripts\\DCS-BIOS\\doc\\doc_assets\\A-4E-C.jsonp
       macOS:   <repo>/Firmware/ScratchPad/DCS-BIOS/DCS-BIOS/doc/doc_assets/A-4E-C.jsonp
  3. Committed snapshot:       tools/gen_a4ec/data/A-4E-C.jsonp
"""

import argparse
import json
import platform
import re
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Optional

SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT  = SCRIPT_DIR.parent.parent
OUTPUT_DIR = REPO_ROOT / "Firmware" / "Libraries" / "A4EC"
COMMITTED_SNAPSHOT = SCRIPT_DIR / "data" / "A-4E-C.jsonp"
LEDGER_PATH        = SCRIPT_DIR / "id_ledger.json"
NODE_STATUS_H      = REPO_ROOT / "Firmware" / "Libraries" / "NodeStatus" / "NodeStatus.h"

DCSIN_BASE = 0x8001
DCSIN_MAX  = 0x86FF

# Dispatch form is sourced from the PanelGroup input class — specifically the CAN frame it emits on
# (ABS canIdEvt / REL canIdEvtRel / DIR canIdEvtDir) — NOT inferred from the JSON here (see #147).
# The generated input map is therefore controlId → name only; PanelBridge formats the payload by
# frame (%u / %+d / INC-DEC).

# ── Source resolution ─────────────────────────────────────────────────────────

def _windows_candidate() -> Path:
    import os
    profile = os.environ.get("USERPROFILE", "")
    return (Path(profile) / "Saved Games" / "DCS" / "Scripts" /
            "DCS-BIOS" / "doc" / "doc_assets" / "A-4E-C.jsonp")

def _mac_candidate() -> Path:
    return (REPO_ROOT / "Firmware" / "ScratchPad" /
            "DCS-BIOS" / "DCS-BIOS" / "doc" / "doc_assets" / "A-4E-C.jsonp")

def resolve_source(explicit: Optional[str]) -> Path:
    if explicit:
        p = Path(explicit)
        if not p.exists():
            sys.exit(f"ERROR: --source path not found: {p}")
        return p

    candidate = _windows_candidate() if platform.system() == "Windows" else _mac_candidate()
    if candidate.exists():
        return candidate

    if COMMITTED_SNAPSHOT.exists():
        return COMMITTED_SNAPSHOT

    sys.exit(
        "ERROR: No A-4E-C.jsonp found.\n"
        "Options:\n"
        "  1. Run with --source <path>\n"
        f"  2. Copy to committed snapshot: cp <source> {COMMITTED_SNAPSHOT}\n"
        "  3. Windows: install DCS-BIOS into Saved Games\\DCS\\Scripts\\\n"
        "     macOS:   ensure ScratchPad is populated from the DCS-BIOS repo"
    )

# ── JSONP / JSON parsing ──────────────────────────────────────────────────────

def load_data(path: Path) -> dict:
    content = path.read_text(encoding="utf-8")
    # Strip JSONP wrapper: first line is `docdata["..."] =`; last char may be `;`
    if content.lstrip().startswith("docdata["):
        content = content.split("\n", 1)[1]
    content = content.rstrip().rstrip(";")
    return json.loads(content)

# ── Flatten nested JSON ───────────────────────────────────────────────────────

def flatten_controls(data: dict) -> dict:
    """Flatten {category: {control_name: data}} → {control_name: data}."""
    out = {}
    for category_controls in data.values():
        for name, ctrl in category_controls.items():
            out[name] = ctrl
    return out

# classify_input was removed in #147: the PanelBridge dispatch form is sourced from the PanelGroup
# input class (via the CAN frame the node emits on), not inferred from the JSON interface set here.
# The map collapses to controlId → name, so every input control maps and nothing is "unsupported".

# ── ID ledger — append-only DCSIN assignment ─────────────────────────────────
# The cmdId for a control is assigned once and never changes: the ledger is the
# durable source of truth, the headers derive from it. New controls append at the
# end (max+1); removed controls keep their id reserved (never reused), so a stale
# sketch reference can't silently resolve to a different control. On first run the
# ledger is absent → controls are numbered alphabetically from DCSIN_BASE (matching
# the historical assignment) and the ledger is created — output is byte-identical.

def load_ledger(path: Path = LEDGER_PATH) -> dict:
    """Load the committed {name: cmdId} ledger; empty dict on first run (auto-seeds)."""
    if path.exists():
        raw = json.loads(path.read_text(encoding="utf-8"))
        return {k: int(v) for k, v in raw.items()}
    return {}

def save_ledger(ledger: dict, path: Path = LEDGER_PATH) -> None:
    """Persist the ledger sorted by id (stable diff). Retired ids stay; they are not popped."""
    ordered = dict(sorted(ledger.items(), key=lambda kv: kv[1]))
    path.write_text(json.dumps(ordered, indent=2) + "\n", encoding="utf-8")

# ── Input pass ────────────────────────────────────────────────────────────────

def build_input_entries(controls: dict, ledger_path: Path = LEDGER_PATH) -> tuple:
    """
    Returns (mapped_entries, gaps).

    Each mapped entry: {name, cmdId}. The dispatch form is sourced from the PanelGroup class
    via the CAN frame (#147), not inferred here, so every input control maps and gaps is always
    empty for inputs.
    """
    candidates = sorted(
        [(n, c) for n, c in controls.items() if c.get("inputs")],
        key=lambda x: x[0],
    )

    if len(candidates) > (DCSIN_MAX - DCSIN_BASE + 1):
        sys.exit(
            f"ERROR: {len(candidates)} input controls exceed DCSIN ID range "
            f"(0x{DCSIN_BASE:04X}–0x{DCSIN_MAX:04X})"
        )

    ledger  = load_ledger(ledger_path)                        # {name: cmdId}, append-only
    next_id = (max(ledger.values()) + 1) if ledger else DCSIN_BASE

    mapped = []
    gaps   = []

    for name, _ in candidates:
        if name in ledger:                                    # keep previously-assigned id
            cmd_id = ledger[name]
        else:                                                 # new control → append at the end
            cmd_id = next_id
            ledger[name] = cmd_id
            next_id += 1
        if cmd_id > DCSIN_MAX:
            sys.exit(f"ERROR: DCSIN id space exhausted assigning {name}")

        mapped.append({"name": name, "cmdId": cmd_id})

    save_ledger(ledger, ledger_path)                          # retired ids stay reserved
    return mapped, gaps

# ── Output pass ───────────────────────────────────────────────────────────────

def build_output_entries(controls: dict) -> tuple:
    """
    Returns (output_entries, gaps).

    Each entry: {addr_id, mask_id, address, mask, comment}
    """
    out  = []
    gaps = []

    for name in sorted(controls):
        ctrl = controls[name]
        for o in ctrl.get("outputs", []):
            addr_id = o.get("address_mask_shift_identifier")
            if not addr_id:
                gaps.append({"name": name, "reason": "no_address_id",
                             "detail": "Output entry missing address_mask_shift_identifier"})
                continue
            mask_id = o.get("address_mask_identifier", addr_id + "_AM")
            out.append({
                "addr_id": addr_id,
                "mask_id": mask_id,
                "address": o["address"],
                "mask":    o["mask"],
                "comment": ctrl.get("description", ""),
            })

    out.sort(key=lambda e: e["addr_id"])
    return out, gaps

# ── Change report (#295) ──────────────────────────────────────────────────────
# DCSIN_* ids are append-only (the ledger) and must never change; A_4E_C_* addresses and masks
# follow DCS-BIOS upstream. A refresh that removes or moves a symbol a sketch may use is listed so
# the maintainer can decide the version bump — additions are the normal case.

_DEFINE_RE = re.compile(r"^#define\s+(\w+)\s+(0x[0-9A-Fa-f]+)", re.M)

def parse_defines(text: str) -> dict:
    """{name: value} for every `#define NAME 0x…` line of a generated header."""
    return {name: int(value, 16) for name, value in _DEFINE_RE.findall(text)}

def diff_defines(old: dict, new: dict) -> dict:
    """Added / removed / moved (same name, new value) symbols between two header versions."""
    return {
        "added":   sorted(set(new) - set(old)),
        "removed": sorted(set(old) - set(new)),
        "moved":   sorted((n, old[n], new[n]) for n in set(old) & set(new) if old[n] != new[n]),
    }

def changed_dcsin_ids(cmd_diff: dict) -> list:
    """DCSIN_* symbols whose id would change — a ledger violation, never allowed."""
    return [m for m in cmd_diff["moved"] if m[0].startswith("DCSIN_")]

def emit_change_report(cmd_diff: dict, out_diff: dict) -> str:
    """Markdown for the refresh PR body: what a sketch built against the old headers loses."""
    removed = cmd_diff["removed"] + out_diff["removed"]
    moved   = out_diff["moved"]
    added   = cmd_diff["added"] + out_diff["added"]
    lines = ["## Symbol changes", ""]
    if not removed and not moved:
        lines.append(f"Additions only ({len(added)} new symbols) — nothing a sketch uses was removed "
                     "or moved.")
        return "\n".join(lines) + "\n"
    warning = ("**Removed or moved symbols** — a sketch using one of these stops compiling (removed) "
               "or silently reads the wrong address (moved). Decide whether this release needs a "
               "major version bump before merging.")
    lines += [warning, ""]
    for name in removed:
        lines.append(f"- removed `{name}`")
    for name, old, new in moved:
        lines.append(f"- moved `{name}`: `{old:#06x}` → `{new:#06x}`")
    lines += ["", f"Added: {len(added)} new symbols."]
    return "\n".join(lines) + "\n"

# ── Reserved DCS-BIOS address / names check (#295) ────────────────────────────
# The client asks PanelBridge for the node roster by writing NODE_STATUS_REQ_ADDR into the DCS-BIOS
# export stream, and PanelBridge answers with _NODE_* command messages. Both only work while no
# DCS-BIOS aircraft uses that address or those names — 0x86FE was moved off for exactly that (the
# AH-64D and Mirage F1 use it). A refresh checks every module in the release and warns if one now
# does. It warns rather than fails: the refresh itself is still fine, the address may need moving.

def read_reserved(node_status_h: Path = NODE_STATUS_H) -> tuple:
    """(roster-request address, {message names}) as NodeStatus.h defines them — the single source."""
    text = node_status_h.read_text(encoding="utf-8")
    addr = int(re.search(r"#define\s+NODE_STATUS_REQ_ADDR\s+(0x[0-9A-Fa-f]+)", text).group(1), 16)
    names = set(re.findall(r'#define\s+NODE_\w+_MSG_NAME\s+"(\w+)"', text))
    return addr, names

def find_reserved_conflicts(modules_dir: Path, addr: int, names: set) -> list:
    """Every DCS-BIOS module output that covers `addr`, and every control named like ours."""
    conflicts = []
    for path in sorted(Path(modules_dir).glob("*.json")):
        try:
            data = json.loads(path.read_text(encoding="utf-8"))
        except (ValueError, UnicodeDecodeError):
            continue
        for category in data.values():
            if not isinstance(category, dict):          # AircraftAliases.json is a different shape
                continue
            for ident, ctrl in category.items():
                if ident in names:
                    conflicts.append(f"`{path.stem}` has a control named `{ident}`")
                for out in ctrl.get("outputs", []):
                    start = out["address"]
                    length = out.get("max_length") if out.get("type") == "string" else None
                    end = start + ((length + 1) // 2 * 2 if length else 2) - 1
                    if start <= addr <= end:
                        conflicts.append(f"`{path.stem}` uses `{addr:#06x}` (`{ident}`)")
    return conflicts

def emit_reserved_report(conflicts: list, addr: int) -> str:
    lines = ["## Reserved address check", ""]
    if not conflicts:
        lines.append(f"No aircraft in this release uses the roster-request address `{addr:#06x}` or "
                     "our `_NODE_*` message names.")
    else:
        lines += [f"**Warning:** the roster request (`NODE_STATUS_REQ_ADDR` = `{addr:#06x}`) or our "
                  "`_NODE_*` names now clash with DCS-BIOS. Flying that aircraft would make the "
                  "bridge send the roster on every update of that control. Move the address "
                  "(`NodeStatus.h`, then client sync) before this reaches the cockpit.", ""]
        lines += [f"- {c}" for c in conflicts]
    return "\n".join(lines) + "\n"

# ── Emission helpers ──────────────────────────────────────────────────────────

def _c_str(val: Optional[str]) -> str:
    return f'"{val}"' if val is not None else "nullptr"

def _now_iso() -> str:
    return datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")

# ── A4EC_CmdIds.h ─────────────────────────────────────────────────────────────

def emit_cmdids(entries: list, source_name: str, ts: str) -> str:
    lines = [
        "// A4EC_CmdIds.h — AUTO-GENERATED by tools/gen_a4ec/gen_a4ec.py",
        "// DO NOT EDIT — regenerate using: python tools/gen_a4ec/gen_a4ec.py",
        f"// DCS-BIOS source: {source_name}",
        f"// Generated:       {ts}",
        f"// Control count:   {len(entries)}",
        "",
        "#pragma once",
        "",
        "// DCSIN_* — compact CAN transport IDs for DCS-BIOS A-4E-C input controls.",
        "// Range: 0x8001 – 0x86FF  (see FirmwarePlan/04-dcs-bios-integration.md).",
        "// IDs are assigned in alphabetical order of the DCS-BIOS control name.",
        "",
    ]
    for e in entries:
        lines.append(f"#define DCSIN_{e['name']:<44} {e['cmdId']:#06x}")
    lines.append("")
    return "\n".join(lines)

# ── A4EC_OutputIds.h ──────────────────────────────────────────────────────────

def emit_outputids(entries: list, source_name: str, ts: str) -> str:
    lines = [
        "// A4EC_OutputIds.h — AUTO-GENERATED by tools/gen_a4ec/gen_a4ec.py",
        "// DO NOT EDIT — regenerate using: python tools/gen_a4ec/gen_a4ec.py",
        f"// DCS-BIOS source: {source_name}",
        f"// Generated:       {ts}",
        f"// Output count:    {len(entries)}",
        "",
        "#pragma once",
        "",
        "// Output address and mask constants for A-4E-C cockpit indicators and gauges.",
        "// Constant names match the DCS-BIOS published identifiers exactly — the same",
        "// names shown in Bort and other DCS-BIOS debug tools.",
        "//",
        "// For each output, two constants are defined:",
        "//   <address_mask_shift_identifier>   — uint16_t DCS-BIOS output address",
        "//   <address_mask_identifier> (_AM)   — uint16_t bitmask for the relevant bits",
        "//",
        "// Usage with LED:",
        "//   OpenSkyhawk::LED fire(A_4E_C_D_GLARE_FIRE, A_4E_C_D_GLARE_FIRE_AM, pin);",
        "",
    ]
    for e in entries:
        comment = f"  // {e['comment']}" if e["comment"] else ""
        lines.append(f"#define {e['addr_id']:<44} {e['address']:#06x}{comment}")
        lines.append(f"#define {e['mask_id']:<44} {e['mask']:#06x}")
        lines.append("")
    return "\n".join(lines)

# ── A4EC_InputMap.h ───────────────────────────────────────────────────────────

def emit_inputmap(entries: list, source_name: str, ts: str) -> str:
    lines = [
        "// A4EC_InputMap.h — AUTO-GENERATED by tools/gen_a4ec/gen_a4ec.py",
        "// DO NOT EDIT — regenerate using: python tools/gen_a4ec/gen_a4ec.py",
        "// Include from PanelBridge only — PanelGroup sketches do not need this file.",
        f"// Generated: {ts}",
        "",
        "#pragma once",
        "#include <A4EC_CmdIds.h>",
        "",
        "// ── Dispatch entry ────────────────────────────────────────────────────────",
        "// controlId → DCS-BIOS name only. The dispatch FORM (absolute / relative-step /",
        "// direction) is sourced from the PanelGroup input class via the CAN frame the node",
        "// emits on — not from this map. PanelBridge formats the payload by frame (#147).",
        "",
        "struct DcsBiosInputEntry {",
        "    uint16_t    cmdId;  ///< DCSIN_* compact ID — matches A4EC_CmdIds.h constant",
        "    const char* name;   ///< DCS-BIOS control name for sendDcsBiosMessage()",
        "};",
        "",
        "// ── Dispatch table — sorted ascending by cmdId ────────────────────────────",
        "// Binary search by PanelBridge.",
        "",
        "static const DcsBiosInputEntry A4EC_INPUT_MAP[] = {",
    ]

    for e in sorted(entries, key=lambda x: x["cmdId"]):
        lines.append(f'    {{ DCSIN_{e["name"]}, "{e["name"]}" }},')

    lines += [
        "};",
        "",
        "static constexpr uint16_t A4EC_INPUT_MAP_SIZE =",
        "    sizeof(A4EC_INPUT_MAP) / sizeof(A4EC_INPUT_MAP[0]);",
        "",
    ]
    return "\n".join(lines)

# ── GENERATOR_GAPS.md ─────────────────────────────────────────────────────────

def emit_gaps(gaps: list, source_name: str, ts: str) -> str:
    lines = [
        "# A4EC Generator Gaps",
        "",
        "Auto-generated by tools/gen_a4ec/gen_a4ec.py — DO NOT EDIT.",
        f"Last run: {ts}",
        f"Source:   {source_name}",
        "",
    ]
    if not gaps:
        lines += [
            "## Unsupported Controls",
            "",
            "*(none — all controls mapped successfully)*",
            "",
        ]
    else:
        lines += [
            "## Unsupported Controls",
            "",
            "| Control name | Reason | Detail |",
            "|---|---|---|",
        ]
        for g in sorted(gaps, key=lambda x: x["name"]):
            lines.append(f"| {g['name']} | {g['reason']} | {g['detail']} |")
        lines.append("")
    return "\n".join(lines)

# ── Main ──────────────────────────────────────────────────────────────────────

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generate OpenSkyhawk A4EC firmware headers from DCS-BIOS JSON."
    )
    parser.add_argument("--source", metavar="PATH",
                        help="Path to A-4E-C.jsonp (or .json); overrides auto-detect")
    parser.add_argument("--timestamp", metavar="VALUE",
                        help="Generation timestamp/label to embed; defaults to current UTC time")
    parser.add_argument("--source-label", metavar="TEXT",
                        help="Source label to embed; defaults to the source filename")
    parser.add_argument("--modules-dir", metavar="DIR",
                        help="DCS-BIOS release doc/json folder; warn if any module uses our reserved address/names")
    parser.add_argument("--change-report", metavar="PATH",
                        help="Write the removed/moved/added symbol report (Markdown) here")
    parser.add_argument("--fetch-github", action="store_true",
                        help="(not yet implemented) Fetch JSON from DCS-Skunkworks/dcs-bios release")
    args = parser.parse_args()

    if args.fetch_github:
        print("--fetch-github is not yet implemented.")
        print("See tools/gen_a4ec/README.md for the intended approach.")
        sys.exit(0)

    source = resolve_source(args.source)
    print(f"Source:  {source}")

    data     = load_data(source)
    controls = flatten_controls(data)
    print(f"Controls loaded: {len(controls)}")

    ts = args.timestamp if args.timestamp else _now_iso()

    input_entries,  input_gaps  = build_input_entries(controls)
    output_entries, output_gaps = build_output_entries(controls)
    all_gaps = input_gaps + output_gaps

    print(f"Inputs  — mapped: {len(input_entries):>4}  skipped: {len(input_gaps)}")
    print(f"Outputs — mapped: {len(output_entries):>4}  skipped: {len(output_gaps)}")

    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    source_name = args.source_label if args.source_label else source.name
    writes = {
        "A4EC_CmdIds.h":    emit_cmdids(input_entries,  source_name, ts),
        "A4EC_OutputIds.h": emit_outputids(output_entries, source_name, ts),
        "A4EC_InputMap.h":  emit_inputmap(input_entries,  source_name, ts),
        "GENERATOR_GAPS.md": emit_gaps(all_gaps, source_name, ts),
    }

    def previous(filename: str) -> dict:
        path = OUTPUT_DIR / filename
        return parse_defines(path.read_text(encoding="utf-8")) if path.exists() else {}

    cmd_diff = diff_defines(previous("A4EC_CmdIds.h"), parse_defines(writes["A4EC_CmdIds.h"]))
    out_diff = diff_defines(previous("A4EC_OutputIds.h"), parse_defines(writes["A4EC_OutputIds.h"]))
    broken = changed_dcsin_ids(cmd_diff)
    if broken:
        for name, old, new in broken:
            print(f"ERROR: {name} would change {old:#06x} -> {new:#06x}", file=sys.stderr)
        sys.exit("ERROR: DCSIN_* ids are append-only (id_ledger.json) — an existing id must never "
                 "change. Restore the ledger from git; do not edit or re-seed it.")

    report = emit_change_report(cmd_diff, out_diff)
    if args.modules_dir:
        req_addr, req_names = read_reserved()
        conflicts = find_reserved_conflicts(Path(args.modules_dir), req_addr, req_names)
        for c in conflicts:
            print(f"WARNING: reserved address/name clash: {c}", file=sys.stderr)
        report += "\n" + emit_reserved_report(conflicts, req_addr)
    print(report.rstrip())
    if args.change_report:
        Path(args.change_report).write_text(report, encoding="utf-8")

    for filename, content in writes.items():
        out_path = OUTPUT_DIR / filename
        out_path.write_text(content, encoding="utf-8")
        print(f"  Wrote {out_path.relative_to(REPO_ROOT)}")

    print("Done.")


if __name__ == "__main__":
    main()
