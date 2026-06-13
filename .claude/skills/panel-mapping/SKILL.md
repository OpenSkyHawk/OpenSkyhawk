---
name: panel-mapping
description: Research a new OpenSkyhawk panel from DCS — identify its controls and control types, confirm DCS-BIOS IDs, build the I/O summary, and measure the panel size from the DCS Model Viewer. Use when starting or researching a new panel, identifying switch/control types, recording Model Viewer corner coordinates, computing panel dimensions/scale, or mapping controls to DCS-BIOS. This is the front of the per-panel pipeline; it feeds firmware, pcb-design, and cad.
---

# Panel Mapping (DCS → control inventory + dimensions)

The first step for any new panel. Produces two outputs — a **control inventory** and the
**panel dimensions** — that feed the rest of the pipeline (`firmware` classes, `pcb-design`
I/O, `cad` outline).

The authoritative worked record lives in the **Notion Panels database** (one page per panel);
the best example is *Misc Switch Panel* — Controls Inventory + I/O Summary + Dimensions tables.
A condensed copy lives in `docs/_source/controllers/<Panel>.md`. Record findings in the Notion
page body (per the CLAUDE.md Notion rules), then mirror the condensed version into the repo.

## Process

1. **Identify controls.** In the DCS **Model Viewer** plus the mod Lua sources — `clickabledata.lua`
   (clickable points), `command_defs.lua`, `A-4E-C.lua` (DCS-BIOS), and system files (e.g.
   `nav.lua`, `shrike.lua`). Record each control's DCS point number and panel label.
2. **Classify the control type** — momentary pushbutton / 2-pos toggle / 3-pos toggle /
   multi-pos rotary / continuous pot / not-in-sim (spare GPIO). The type drives **both**:
   - firmware class (`firmware`): `Switch2Pos`, `Switch3Pos`, `RotarySwitch`/`SwitchMultiPos`,
     `AnalogInput`, `ActionButton`, `RotaryEncoder`, …
   - PCB I/O (`pcb-design`): GPIO count (3-pos = 2 GPIO, n-pos rotary = n GPIO), or an ADC
     channel for a pot.
3. **Confirm the DCS-BIOS ID** and positions/range and device from `A-4E-C.lua` / the system
   Lua. Flag DCS **stubs / unimplemented handlers** and check the community-a4e-c GitHub repo
   for related issues. Note the two ID schemes — Model-Viewer/Lua native IDs vs DCS-BIOS
   addresses — and don't conflate them (`docs/_source/controllers/a4e-c-dcsbios-addresses.md`).
4. **Build the I/O Summary:** type → count → implementation → total GPIO/ADC budget → does it
   fit one MCP23017 (14 inputs) or need more / an ADS1115.
5. **Dimensions:** record the panel corner coordinates (X, Y, Z) from the Model Viewer.
   **1 model unit = 1000 mm**, then **× 1.10 scale**:
   `real-world mm = √(ΔX² + ΔY² + ΔZ²) × 1.10 × 1000`
   (width = bottom-left → bottom-right, height = bottom-left → top-left). Switch
   centre-to-centre spacings are measured later in the CAD model.
6. **Record** to the Notion Panels page (Controls Inventory / I/O Summary / Dimensions) and the
   condensed `docs/_source/controllers/<Panel>.md`. Claim a NODE_ID in `Firmware/NODE_IDS.md`
   when the panel becomes a PanelGroup node.

## Hand-off

- control inventory + types → `firmware` (which classes) and `pcb-design` (GPIO/ADC allocation)
- panel dimensions → `cad` (model) and `pcb-design` (board outline)
