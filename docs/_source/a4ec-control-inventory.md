# A-4E-C control inventory (reference)

`a4ec-control-inventory.csv` (this directory) is the **curated master list of every controllable
element in the A-4E-C cockpit** — one row per control. It is the front-of-pipeline reference data
for panel research and controller building; **git holds this CSV, the GitHub Projects hold the
per-panel assignment + build state.**

## When to use it

Read this CSV whenever you need the ground truth for a control — building a panel's I/O, picking a
firmware class, wiring a schematic, or estimating a controller's budget. It answers, per control:
*what it is, how it's driven, where it routes, and which OpenSkyhawk building block it maps to.*

## Columns

| Column | Meaning |
|---|---|
| `kind` | `input` (clickable cockpit control) · `output` (gauge / LED) · `hotas` (HID flight control) |
| `dcs_category` | **curated** system/panel grouping (e.g. `Main Flight Instruments`, `Engine Instruments`, `Warning Lamps`, `T Handles`, `Radar Scope`). Authoritative — use as the grouping seed; **not** the raw DCS-BIOS category. |
| `native_id` | model-viewer / clickabledata ID (the join key to the mod files) |
| `identifier` | DCS-BIOS identifier, e.g. `ARM_EMERG_SEL` |
| `name` | human description |
| `phys_type` | clickabledata constructor (`default_2_position_tumb`, `multiposition_switch_limited`, `default_axis_limited`, `gauge`, `LED`, …) |
| `positions` | rotary position count |
| `routing` | `DCS-BIOS` (record `address`) · `HID` / `HID/keybind` (consumes GPIO/ADC, different firmware path) |
| `address` | DCS-BIOS address (DCSIN ≥ 0x8000 → PanelBridge) |
| `value_range` | gauge `max_value` (output range → stepper steps) |
| `ifaces` | DCS-BIOS input interface palette (`set_state` / `fixed_step` / `variable_step` / `action`) — the methods a control supports; hardware picks one at schematic time |
| `fw_class` | the OpenSkyhawk firmware class it maps to (`Switch2Pos`, `SwitchMultiPos`, `AnalogInput`, `SwitecX25Output`, `LED`, …) |

## Caveats

- **Identifier prefix ≠ physical panel.** A control's panel/console comes from the **GitHub Project**
  (Console field), not from the identifier or `dcs_category`. Some controls sit on a different panel
  than their prefix implies (e.g. `BDHI_MODE`, `RADAR_PROFILE/RANGE` are on the Misc Switch Panel).
- **Rotaries aren't locked to one class** — `fw_class` shows the default; >7 positions → resistor
  ladder (`AnalogMultiPos`), ≤7 is a per-panel judgment call. The pick is made at schematic time.
- **Gauges are composites** — multi-element instruments (ADI, altimeter, nav drums) are several
  `SwitecX25Output` steppers grouped by identifier prefix.

## Regenerating

`tools/a4ec_inventory/a4ec_inventory.py` produces a **first cut** from the A-4E-C mod Lua + DCS-BIOS
(see its README). This CSV is that output **after human curation** (better `dcs_category` groups,
external-model rows dropped). Re-run the generator for a fresh first cut, then re-curate — don't
hand-edit blindly.
