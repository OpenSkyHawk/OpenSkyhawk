#!/usr/bin/env python3
"""
gen_a4ec.py — OpenSkyhawk A4EC header generator.

Parses the DCS-BIOS A-4E-C.jsonp control definitions and emits:
  Firmware/Libraries/A4EC/A4EC_CmdIds.h      — DCSIN_* compact CAN transport IDs
  Firmware/Libraries/A4EC/A4EC_OutputIds.h   — A_4E_C_* output address/mask constants
  Firmware/Libraries/A4EC/A4EC_InputMap.h    — DcsBiosInputEntry[] dispatch table
  Firmware/Libraries/A4EC/GENERATOR_GAPS.md  — skipped controls with reasons

Usage (from repo root):
  python Tools/gen_a4ec/gen_a4ec.py [--source <path>] [--fetch-github]

Source resolution (priority order):
  1. --source <path>           explicit override, any platform
  2. Platform auto-detect:
       Windows: %USERPROFILE%\\Saved Games\\DCS\\Scripts\\DCS-BIOS\\doc\\doc_assets\\A-4E-C.jsonp
       macOS:   <repo>/Firmware/ScratchPad/DCS-BIOS/DCS-BIOS/doc/doc_assets/A-4E-C.jsonp
  3. Committed snapshot:       Tools/gen_a4ec/data/A-4E-C.jsonp
"""

import argparse
import json
import platform
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Optional

SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT  = SCRIPT_DIR.parent.parent
OUTPUT_DIR = REPO_ROOT / "Firmware" / "Libraries" / "A4EC"
COMMITTED_SNAPSHOT = SCRIPT_DIR / "data" / "A-4E-C.jsonp"

DCSIN_BASE = 0x8001
DCSIN_MAX  = 0x86FF

# ── Input type constants (mirrored in emitted A4EC_InputMap.h) ────────────────

INPUT_TYPE_SWITCH        = 0
INPUT_TYPE_ACTION        = 1
INPUT_TYPE_ENCODER       = 2
INPUT_TYPE_ACCEL_ENCODER = 3
INPUT_TYPE_MULTIPOS      = 4
INPUT_TYPE_ANALOG        = 5

_TYPE_NAMES = {
    INPUT_TYPE_SWITCH:        "InputType::SWITCH",
    INPUT_TYPE_ACTION:        "InputType::ACTION",
    INPUT_TYPE_ENCODER:       "InputType::ENCODER",
    INPUT_TYPE_ACCEL_ENCODER: "InputType::ACCEL_ENCODER",
    INPUT_TYPE_MULTIPOS:      "InputType::MULTIPOS",
    INPUT_TYPE_ANALOG:        "InputType::ANALOG",
}

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

# ── Input classification ──────────────────────────────────────────────────────

def classify_input(name: str, inputs: list) -> dict:
    """
    Classify a control into an InputType.

    Returns a dict with keys: type, arg0, arg1, arg0fast, arg1fast.
    Returns a dict with key _skip=True on failure, plus _reason and _detail.
    """
    interface_set = {i["interface"] for i in inputs}

    # variable_step: continuous analog dial — maps to AnalogInput
    if "variable_step" in interface_set:
        return {"type": INPUT_TYPE_ANALOG,
                "arg0": None, "arg1": None, "arg0fast": None, "arg1fast": None}

    fixed_steps = [i for i in inputs if i["interface"] == "fixed_step"]
    set_states  = [i for i in inputs if i["interface"] == "set_state"]
    actions     = [i for i in inputs if i["interface"] == "action"]

    # Two fixed_step entries → ACCEL_ENCODER
    if len(fixed_steps) == 2:
        slow, fast = fixed_steps
        return {
            "type":     INPUT_TYPE_ACCEL_ENCODER,
            "arg0":     slow.get("argument_decrement", "DEC"),
            "arg1":     slow.get("argument_increment", "INC"),
            "arg0fast": fast.get("argument_decrement", "DEC"),
            "arg1fast": fast.get("argument_increment", "INC"),
        }

    # set_state present → SWITCH (max=1) or MULTIPOS (max>1)
    if set_states:
        max_val = set_states[0].get("max_value", 1)
        if max_val == 1:
            return {"type": INPUT_TYPE_SWITCH,
                    "arg0": "0", "arg1": "1", "arg0fast": None, "arg1fast": None}
        return {"type": INPUT_TYPE_MULTIPOS,
                "arg0": None, "arg1": None, "arg0fast": None, "arg1fast": None}

    # action only (no set_state)
    if actions:
        return {"type": INPUT_TYPE_ACTION,
                "arg0": actions[0].get("argument", "1"),
                "arg1": None, "arg0fast": None, "arg1fast": None}

    # Single fixed_step only → ENCODER
    if len(fixed_steps) == 1:
        return {"type": INPUT_TYPE_ENCODER,
                "arg0": "DEC", "arg1": "INC", "arg0fast": None, "arg1fast": None}

    return {"_skip": True, "_reason": "unsupported",
            "_detail": f"Unrecognised interface combination: {sorted(interface_set)}"}

# ── Input pass ────────────────────────────────────────────────────────────────

def build_input_entries(controls: dict) -> tuple:
    """
    Returns (mapped_entries, gaps).

    Each mapped entry: {name, cmdId, type, arg0, arg1, arg0fast, arg1fast}
    Each gap:          {name, reason, detail}
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

    mapped = []
    gaps   = []
    cmd_id = DCSIN_BASE

    for name, ctrl in candidates:
        result = classify_input(name, ctrl["inputs"])
        if result.get("_skip"):
            gaps.append({"name": name,
                         "reason": result["_reason"],
                         "detail": result["_detail"]})
        else:
            mapped.append({
                "name":     name,
                "cmdId":    cmd_id,
                "type":     result["type"],
                "arg0":     result["arg0"],
                "arg1":     result["arg1"],
                "arg0fast": result["arg0fast"],
                "arg1fast": result["arg1fast"],
            })
            cmd_id += 1

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

# ── Emission helpers ──────────────────────────────────────────────────────────

def _c_str(val: Optional[str]) -> str:
    return f'"{val}"' if val is not None else "nullptr"

def _now_iso() -> str:
    return datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")

# ── A4EC_CmdIds.h ─────────────────────────────────────────────────────────────

def emit_cmdids(entries: list, source_name: str, ts: str) -> str:
    lines = [
        "// A4EC_CmdIds.h — AUTO-GENERATED by Tools/gen_a4ec/gen_a4ec.py",
        "// DO NOT EDIT — regenerate using: python Tools/gen_a4ec/gen_a4ec.py",
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
        "// A4EC_OutputIds.h — AUTO-GENERATED by Tools/gen_a4ec/gen_a4ec.py",
        "// DO NOT EDIT — regenerate using: python Tools/gen_a4ec/gen_a4ec.py",
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
        "//   OpenSkyhawk::LED warn(A_4E_C_MASTER_CAUTION, A_4E_C_MASTER_CAUTION_AM, pin);",
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
        "// A4EC_InputMap.h — AUTO-GENERATED by Tools/gen_a4ec/gen_a4ec.py",
        "// DO NOT EDIT — regenerate using: python Tools/gen_a4ec/gen_a4ec.py",
        "// Include from PanelBridge only — PanelGroup sketches do not need this file.",
        f"// Generated: {ts}",
        "",
        "#pragma once",
        "#include <A4EC_CmdIds.h>",
        "",
        "// ── Input dispatch type codes ─────────────────────────────────────────────",
        "",
        "namespace InputType {",
        "    static constexpr uint8_t SWITCH        = 0;  ///< Switch2Pos — sends 0 or 1",
        "    static constexpr uint8_t ACTION        = 1;  ///< ActionButton — sends 1 (press only)",
        "    static constexpr uint8_t ENCODER       = 2;  ///< RotaryEncoder — sends 0=CCW, 1=CW",
        "    static constexpr uint8_t ACCEL_ENCODER = 3;  ///< RotaryAcceleratedEncoder — 0–3",
        "    static constexpr uint8_t MULTIPOS      = 4;  ///< SwitchMultiPos / RotarySwitch / AnalogMultiPos",
        "    static constexpr uint8_t ANALOG        = 5;  ///< AnalogInput — sends raw 0–65535 ADC reading",
        "}",
        "",
        "// ── Dispatch entry ────────────────────────────────────────────────────────",
        "",
        "struct DcsBiosInputEntry {",
        "    uint16_t    cmdId;      ///< DCSIN_* compact ID — matches A4EC_CmdIds.h constant",
        "    uint8_t     type;       ///< One of InputType::* — determines PanelBridge dispatch",
        "    const char* name;       ///< DCS-BIOS control name for sendDcsBiosMessage()",
        '    const char* arg0;       ///< value=0 argument ("0", "DEC", position string)',
        '    const char* arg1;       ///< value=1 argument ("1", "INC"); nullptr for ACTION/MULTIPOS',
        "    const char* arg0fast;   ///< ACCEL_ENCODER: fast decrement arg; nullptr for all other types",
        "    const char* arg1fast;   ///< ACCEL_ENCODER: fast increment arg; nullptr for all other types",
        "};",
        "",
        "// ── Dispatch table — sorted ascending by cmdId ────────────────────────────",
        "// Binary search by PanelBridge. Flash cost ~6–8 KB. RAM cost: zero.",
        "",
        "static const DcsBiosInputEntry A4EC_INPUT_MAP[] = {",
    ]

    for e in entries:
        row = (
            f'    {{ DCSIN_{e["name"]}, '
            f'{_TYPE_NAMES[e["type"]]}, '
            f'"{e["name"]}", '
            f'{_c_str(e["arg0"])}, '
            f'{_c_str(e["arg1"])}, '
            f'{_c_str(e["arg0fast"])}, '
            f'{_c_str(e["arg1fast"])} }},'
        )
        lines.append(row)

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
        "Auto-generated by Tools/gen_a4ec/gen_a4ec.py — DO NOT EDIT.",
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
    parser.add_argument("--fetch-github", action="store_true",
                        help="(not yet implemented) Fetch JSON from DCS-Skunkworks/dcs-bios release")
    args = parser.parse_args()

    if args.fetch_github:
        print("--fetch-github is not yet implemented.")
        print("See Tools/gen_a4ec/README.md for the intended approach.")
        sys.exit(0)

    source = resolve_source(args.source)
    print(f"Source:  {source}")

    data     = load_data(source)
    controls = flatten_controls(data)
    print(f"Controls loaded: {len(controls)}")

    ts = _now_iso()

    input_entries,  input_gaps  = build_input_entries(controls)
    output_entries, output_gaps = build_output_entries(controls)
    all_gaps = input_gaps + output_gaps

    print(f"Inputs  — mapped: {len(input_entries):>4}  skipped: {len(input_gaps)}")
    print(f"Outputs — mapped: {len(output_entries):>4}  skipped: {len(output_gaps)}")

    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    source_name = source.name
    writes = {
        "A4EC_CmdIds.h":    emit_cmdids(input_entries,  source_name, ts),
        "A4EC_OutputIds.h": emit_outputids(output_entries, source_name, ts),
        "A4EC_InputMap.h":  emit_inputmap(input_entries,  source_name, ts),
        "GENERATOR_GAPS.md": emit_gaps(all_gaps, source_name, ts),
    }

    for filename, content in writes.items():
        out_path = OUTPUT_DIR / filename
        out_path.write_text(content, encoding="utf-8")
        print(f"  Wrote {out_path.relative_to(REPO_ROOT)}")

    print("Done.")


if __name__ == "__main__":
    main()
