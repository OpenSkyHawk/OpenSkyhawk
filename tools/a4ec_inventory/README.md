# a4ec_inventory — A-4E-C master control inventory

Python 3 script that builds a single **row-per-control inventory** of the A-4E-C
cockpit by joining the Community Mod Lua with the DCS-BIOS control definitions.
It is the front-of-pipeline data source for the `panel-pipeline` skill (stage A1):
every controllable element classified by physical type, control method, routing,
and the OpenSkyhawk firmware class it maps to.

Unlike `gen_a4ec` (which emits committed build headers), this tool produces a
**reference CSV** — it is not a compile-time dependency.

## Output columns

| Column | Contents |
|---|---|
| `kind` | `input` (clickable control) · `output` (gauge/LED) · `hotas` (HID flight control) |
| `dcs_category` | DCS-BIOS category (⚠️ **not** the physical panel — e.g. AWRS controls sit under "Armament Panel"). Multi-element gauges carry a `[group]` tag |
| `native_id` | model-viewer / clickabledata ID = the join key |
| `identifier` | DCS-BIOS identifier (e.g. `ARM_EMERG_SEL`) |
| `name` | human description |
| `phys_type` | clickabledata constructor (`default_2_position_tumb`, `multiposition_switch_limited`, `default_axis_limited`, …) |
| `positions` | rotary position count |
| `routing` | `DCS-BIOS` · `HID` · `HID/keybind` |
| `address` | DCS-BIOS output address |
| `value_range` | gauge `max_value` (output range → stepper steps) |
| `ifaces` | DCS-BIOS input interfaces = the **method palette** (`set_state`, `fixed_step`, `variable_step`, `action`) |
| `fw_class` | mapped OpenSkyhawk firmware class |

## Sources & the join

All three live under the A4EC mod + DCS-BIOS (kept in `Firmware/ScratchPad/`, which
is **git-ignored** — these are third-party files, not committed). Pass `--mod` /
`--dcsbios` to point elsewhere.

| File | Provides |
|---|---|
| `<mod>/Cockpit/Scripts/clickabledata.lua` | physical control type (constructor) + native ID |
| `<dcsbios>/lib/modules/aircraft_modules/A-4E-C.lua` | `define<Type>("ID", dev, cmd, ARG, …)` — `ARG` = native ID |
| `<dcsbios>/doc/json/A-4E-C.json` | address + control_type + input interfaces; gauges + LEDs (outputs) |

**Join key = the native model-viewer ID** (clickabledata `PNT_<id>` == module `ARG`).
A clickable control with no module entry is HID/keybind-routed.

## Running

From the repo root (defaults assume the mod + DCS-BIOS are unpacked under
`Firmware/ScratchPad/`):

```bash
python tools/a4ec_inventory/a4ec_inventory.py --out inventory.csv --summary
```

Explicit source paths:

```bash
python tools/a4ec_inventory/a4ec_inventory.py \
  --mod "/path/to/A-4E-C" \
  --dcsbios "/path/to/DCS-BIOS" \
  --out inventory.csv --summary
```

`--out -` (default) writes the CSV to stdout; `--summary` prints counts to stderr.

## Classification notes & caveats

- **Rotaries are not auto-locked to one class.** DCS-BIOS exposes both `fixed_step`
  (encoder) and `set_state` (absolute) on every selector, so a multi-position
  control can be built as a switch (`SwitchMultiPos`), encoder (`RotaryEncoder`), or
  resistor ladder (`AnalogMultiPos`). `fw_class` shows the default; **>7 positions →
  ladder**, ≤7 is a per-panel judgment call. The pick is made at schematic time.
- **One control = a method palette.** The `ifaces` column lists every DCS-BIOS method
  the control supports; the hardware picks one, which determines both the firmware
  class and the command the firmware sends.
- **Gauges are composites.** Multi-element instruments (ADI = 6, altimeter ≈ 7,
  nav digit-drums = 5–6 each) are grouped by the `[group]` tag — each member is one
  stepper (`SwitecX25Output`).
- **`dcs_category` ≠ physical panel.** Use it only as a proxy; the real
  panel assignment comes from the DCS Model Viewer (panel-pipeline A2/B1).
- **HOTAS rows are curated.** Stick / throttle / rudder controls are keybinds in
  `Input/joystick/default.lua` (HID, not clickable), listed here by hand.
- **Confirm-in-sim set.** Push-to-set knobs (`default_button_axis`) and the exact
  rotary method warrant a Model Viewer / in-sim check before the build commits.
