#!/usr/bin/env python3
"""a4ec_inventory — build the A-4E-C master control inventory.

Joins the A-4E-C Community Mod cockpit/input Lua with the DCS-BIOS control
definitions into one row-per-control table (inputs + outputs + HOTAS), classified
by physical type, control method, routing, and the OpenSkyhawk firmware class it
maps to. Front-of-pipeline data for the panel-pipeline skill (stage A1).

Sources (all under the A4EC mod + DCS-BIOS, kept in Firmware/ScratchPad/, which is
git-ignored — pass --mod / --dcsbios to point elsewhere):

  clickabledata.lua          physical control type (constructor) + native model-viewer ID
  <module>/A-4E-C.lua        define<Type>("ID", dev, cmd, ARG, ...) — ARG = native ID = JOIN KEY
  doc/json/A-4E-C.json       DCS-BIOS address + control_type + input interfaces; gauges + LEDs (outputs)

The native model-viewer ID (clickabledata PNT_<id> == module ARG) is the join key
across all three. A clickable control with no module entry is HID/keybind-routed.

Output: CSV (one row per control). See README.md for the column schema and the
classification rules / caveats (rotary multi-method, gauge composition, the
"confirm-in-sim" set).
"""
import argparse
import collections
import csv
import json
import os
import re
import sys

# ── physical-control-type → OpenSkyhawk firmware class ────────────────────────
# Rotaries are intentionally NOT auto-locked to one class: DCS-BIOS exposes both
# fixed_step (encoder) and set_state (switch/ladder) on every selector, so the
# build picks the method at schematic time. >7 positions defaults to AnalogMultiPos
# (resistor ladder, 1 ADC) per the controller-grouping heuristic; <=7 is a judgment
# call (SwitchMultiPos GPIO vs ladder) by panel density.
def fw_class(ctor, n_pos):
    if ctor == "default_2_position_tumb":
        return "Switch2Pos"
    if "3_pos" in ctor or "3_position" in ctor:
        return "Switch3Pos"
    if "multiposition" in ctor:
        return "AnalogMultiPos(>7)" if (n_pos and n_pos > 7) else "SwitchMultiPos/enc/ladder"
    if ctor == "default_button":
        return "ActionButton"
    if ctor == "default_button_axis":  # push-to-set knob: rotate + push
        return "PushSetKnob"
    if "axis" in ctor:
        return "AnalogInput"
    return "?"


def parse_clickabledata(path):
    """native_id -> {desc, ctor}. Skips commented-out lines."""
    out = {}
    if not os.path.isfile(path):
        sys.exit(f"clickabledata not found: {path}")
    with open(path, encoding="utf-8", errors="replace") as fh:
        for line in fh:
            s = line.strip()
            if s.startswith("--"):
                continue
            m = re.match(r'elements\[".*?"\]\s*=\s*([A-Za-z0-9_]+)\s*\(\s*"([^"]*)"(.*)', s)
            if not m:
                continue
            ctor, desc, rest = m.group(1), m.group(2), m.group(3)
            nums = re.findall(r"(?<![\w.])(\d+)(?![\w.])", rest)
            if nums:
                out[int(nums[0])] = {"desc": desc, "ctor": ctor}
    return out


def parse_module(path):
    """native_id (ARG) -> {identifier, define, n_pos, category}."""
    out = {}
    if not os.path.isfile(path):
        sys.exit(f"DCS-BIOS module lua not found: {path}")
    with open(path, encoding="utf-8", errors="replace") as fh:
        for line in fh:
            m = re.search(r':(define\w+)\(\s*"([^"]+)"\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)([^)]*)', line)
            if not m:
                continue
            define, ident, arg, rest = m.group(1), m.group(2), int(m.group(5)), m.group(6)
            nums = re.findall(r"(\d+)", rest)
            strs = re.findall(r'"([^"]+)"', rest)
            out[arg] = {
                "identifier": ident,
                "define": define,
                "n_pos": int(nums[0]) if (define == "defineMultipositionSwitch" and nums) else None,
                "category": strs[-2] if len(strs) >= 2 else "?",
            }
    return out


def parse_json(path):
    """Return (inputs_by_identifier, output_rows). Inputs have an 'inputs' block;
    pure-output controls (gauges/LEDs/metadata) do not."""
    if not os.path.isfile(path):
        sys.exit(f"DCS-BIOS JSON not found: {path}")
    with open(path, encoding="utf-8", errors="replace") as fh:
        data = json.load(fh)
    inputs, outputs = {}, []
    for cat, ctrls in data.items():
        for ident, c in ctrls.items():
            addr = None
            if c.get("outputs") and "address" in c["outputs"][0]:
                addr = c["outputs"][0]["address"]
            if c.get("inputs"):
                inputs[ident] = {
                    "addr": addr,
                    "ifaces": ",".join(i.get("interface", "") for i in c["inputs"]),
                }
            elif c.get("outputs"):
                outputs.append({
                    "ident": ident, "addr": addr, "ctype": c.get("control_type"),
                    "max": c["outputs"][0].get("max_value"), "cat": cat,
                    "desc": c.get("description", ""),
                })
    return inputs, outputs


# Multi-element gauge instruments — each member is one stepper; a gauge is a
# composite of these. Grouping is by identifier prefix (heuristic — review the
# [group] tag in the category column).
GAUGE_GROUPS = [
    "ADI", "ATTGYRO_STBY", "D_ALT", "ALT_ADJ", "D_IAS", "NAV_DEST_LAT", "NAV_DEST_LON",
    "NAV_CURPOS_LAT", "NAV_CURPOS_LON", "ASN41_MAGVAR", "ASN41_WINDDIR", "ASN41_WINDSPEED",
    "APN153_SPEED", "ARC51_FREQ", "CURRTIME", "STOPWATCH", "CM_BANK1", "CM_BANK2",
    "BDHI", "AFCS_HDG", "ACCEL",
]

# HOTAS / flight controls — HID-routed (SimGateway), not clickable cockpit elements
# (they are keybinds in Input/joystick/default.lua). Curated; see README.
HOTAS = [
    ("Gun-Rocket Trigger", "button", "Control Stick"),
    ("Bomb Release (Pickle)", "button", "Control Stick"),
    ("Trim Hat (4-way + reset)", "hat", "Control Stick"),
    ("Autopilot Override Paddle", "button", "Control Stick"),
    ("Stick Roll Axis", "axis", "Control Stick"),
    ("Stick Pitch Axis", "axis", "Control Stick"),
    ("Throttle Axis", "axis", "Throttle"),
    ("Throttle Detent OFF/IGN/RUN", "3pos", "Throttle Grip"),
    ("Speedbrake Switch", "switch", "Throttle Grip"),
    ("Master Exterior Lights (grip)", "switch", "Throttle Grip"),
    ("Rudder Axis", "axis", "Rudder Pedals"),
    ("Wheel Brake Left (toe)", "axis", "Rudder Pedals"),
    ("Wheel Brake Right (toe)", "axis", "Rudder Pedals"),
]

COLS = ["kind", "dcs_category", "native_id", "identifier", "name", "phys_type",
        "positions", "routing", "address", "value_range", "ifaces", "fw_class"]


def gauge_group(ident):
    for g in GAUGE_GROUPS:
        if ident.startswith(g):
            return g
    return ""


def build(mod_dir, dcsbios_dir):
    click = parse_clickabledata(os.path.join(mod_dir, "Cockpit", "Scripts", "clickabledata.lua"))
    mod = parse_module(os.path.join(dcsbios_dir, "lib", "modules", "aircraft_modules", "A-4E-C.lua"))
    jin, jout = parse_json(os.path.join(dcsbios_dir, "doc", "json", "A-4E-C.json"))

    rows = []
    for nid, c in sorted(click.items()):
        m = mod.get(nid)
        ident = m["identifier"] if m else None
        a = jin.get(ident, {}) if ident else {}
        n = m["n_pos"] if m else None
        rows.append({
            "kind": "input", "native_id": nid, "identifier": ident or "", "name": c["desc"],
            "phys_type": c["ctor"], "positions": n or "",
            "routing": "DCS-BIOS" if ident else "HID/keybind",
            "address": ("0x%04X" % a["addr"]) if a.get("addr") else "",
            "ifaces": a.get("ifaces", ""), "fw_class": fw_class(c["ctor"], n),
            "dcs_category": (m["category"] if m else ""),
        })
    for o in jout:
        is_led = o["ctype"] == "led"
        is_meta = o["ctype"] == "metadata"
        rows.append({
            "kind": "output", "native_id": "", "identifier": o["ident"], "name": o["desc"],
            "phys_type": "LED" if is_led else "external-model" if is_meta else "gauge",
            "positions": "", "routing": "DCS-BIOS",
            "address": ("0x%04X" % o["addr"]) if o["addr"] else "",
            "value_range": o.get("max", ""), "ifaces": "",
            "fw_class": "LED" if is_led else "external(excl)" if is_meta else "SwitecX25Output",
            "dcs_category": o["cat"] + ("  [" + gauge_group(o["ident"]) + "]" if gauge_group(o["ident"]) else ""),
        })
    for nm, ty, grp in HOTAS:
        rows.append({
            "kind": "hotas", "native_id": "", "identifier": "", "name": nm, "phys_type": ty,
            "positions": "", "routing": "HID", "address": "", "ifaces": "",
            "fw_class": "AnalogInput/HID" if ty == "axis" else "HIDButton", "dcs_category": grp,
        })
    return rows


def write_csv(rows, out):
    f = open(out, "w", newline="") if out != "-" else sys.stdout
    w = csv.DictWriter(f, fieldnames=COLS, extrasaction="ignore")
    w.writeheader()
    for r in rows:
        w.writerow(r)
    if out != "-":
        f.close()


def print_summary(rows):
    inp = [r for r in rows if r["kind"] == "input"]
    out = [r for r in rows if r["kind"] == "output"]
    hot = [r for r in rows if r["kind"] == "hotas"]
    g = [r for r in out if r["phys_type"] == "gauge"]
    led = [r for r in out if r["phys_type"] == "LED"]
    meta = [r for r in out if r["phys_type"] == "external-model"]
    print(f"total {len(rows)}  (inputs {len(inp)}, outputs {len(out)} "
          f"[gauges {len(g)}, LEDs {len(led)}, external-model {len(meta)}], hotas {len(hot)})", file=sys.stderr)
    print("inputs by fw-class:", dict(collections.Counter(r["fw_class"] for r in inp)), file=sys.stderr)
    print("input routing:", dict(collections.Counter(r["routing"] for r in inp)), file=sys.stderr)


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    repo = os.path.abspath(os.path.join(here, "..", ".."))
    sp = os.path.join(repo, "Firmware", "ScratchPad")
    ap = argparse.ArgumentParser(description="Build the A-4E-C master control inventory CSV.")
    ap.add_argument("--mod", default=os.path.join(sp, "Community_A-4E-C_v2.3 (1)", "Mods", "aircraft", "A-4E-C"),
                    help="A-4E-C mod aircraft dir (contains Cockpit/Scripts + Input)")
    ap.add_argument("--dcsbios", default=os.path.join(sp, "DCS-BIOS", "DCS-BIOS"),
                    help="DCS-BIOS dir (contains doc/json + lib/modules/aircraft_modules)")
    ap.add_argument("--out", default="-", help="output CSV path, or - for stdout (default)")
    ap.add_argument("--summary", action="store_true", help="print a summary to stderr")
    args = ap.parse_args()

    rows = build(args.mod, args.dcsbios)
    write_csv(rows, args.out)
    if args.summary:
        print_summary(rows)


if __name__ == "__main__":
    main()
