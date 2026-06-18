---
name: pcb-design
description: KiCad schematic capture and PCB layout for OpenSkyhawk boards. Use when selecting components or packages, choosing connectors, designing or reviewing a schematic, routing a PCB, setting net classes / track widths / DRC, wiring an MCP23017 or ADS1115 expander, placing LEDs / MOSFET backlight zones / power regulators / steppers, scaffolding a new KiCad project, or running ERC/DRC and gerber/BOM export. Owns the electrical wiring and layout-verification rules.
---

# PCB Design (KiCad + JLCPCB)

Schematic + layout for OpenSkyhawk boards. Each physical PCB is its own KiCad project under
`PCB/<Console>/<Group>/<Board>/`; `PCB/Libraries/` holds shared symbols, footprints, and
hierarchical sheet templates.

## Canonical references — read before non-trivial work

- `docs/_source/hardware-standards.md` — component/package rules, MCU, power, connectors, LED
  backlighting, stepper, gauges, and the **Component Pin Assignment Rules**.
- `docs/_source/pcb-design-rules.md` — JLCPCB 2-layer constraints, net classes, predefined
  track/via sizes, board power budget, placement rules, custom DRC file.
- `docs/_source/kicad.md` — shared library setup, custom symbols/footprints, scaffolding, CLI.

## Scaffolding

Never hand-create project files. Run the `/new-kicad-project <Console> <Group> <Board> [mcu|breakout]`
command — it copies the lib tables, pre-loads JLCPCB design rules + net classes, writes a
minimal root schematic, and updates the panel's GitHub Project item.

## Non-negotiable rules (functional failures if violated — verify at schematic time)

- **MCP23017 GPA7 / GPB7 are the 8th pin of each bank and must be OUTPUTS ONLY — never wire
  them as inputs** (confirmed silicon bug: SDA can corrupt if the pin changes during an I²C
  bit). Route switches/inputs to GPA0–GPA6 and GPB0–GPB6 → **14 inputs per chip**. GPA7/GPB7
  may drive LEDs/enables.
- **Package inspectability:** every IC must be visually inspectable after T962 reflow — SOIC,
  SSOP, TSSOP, HTSSOP, LQFP, SOT-23, SOT-223, through-hole only. **No QFN, DFN, WSON, BGA,
  LGA** or any fully bottom-terminated package. (E.g. the stepper driver is the `DRV8833` in an
  inspectable TSSOP/HTSSOP package — `PW` or `PWP` suffix — never the QFN `RTY` variant.)
- **Never use a linear regulator for 12 V → 5 V.** 5 V → 3.3 V uses AMS1117-3.3 (SOT-223);
  12 V → 5 V (only on high-current boards) uses the AP63205 buck.
- **Placement:** LEDs on the front face only; all other parts (ICs, passives, connectors,
  MOSFETs) on the back face. PTH parts must not conflict with front LED pads.
- **Power budget:** logic + LED boards stay ≤ 500 mA at 12 V input. Actuator boards need a
  separate design review (see `pcb-design-rules.md`).

## I/O wiring quick map (from a panel's control inventory)

A panel's control inventory (built by the `panel-mapping` skill) drives the I/O:

- digital switch/button → MCP23017 GPIO (14 inputs/chip; 3-pos = 2 GPIO; n-pos rotary = n GPIO)
- continuous pot / analog axis → ADS1115 channel (or STM32 ADC) with a 1 kΩ + 100 nF RC filter
- LED zone → IRLML2502 low-side MOSFET, gate from STM32 3.3 V PWM; 5-LED series strings, one
  current-limiting resistor per string (120 Ω default)

I²C addressing: MCP23017 0x20–0x27 (up to 8/bus), ADS1115 0x48–0x4B (up to 4/bus); STM32 has
two I²C buses. The *firmware* side of these parts (driver classes) belongs to the `firmware`
skill — this skill owns the wiring and verification.

## Verify

`kicad-cli sch erc` (clean) and `kicad-cli pcb drc` (clean against the JLCPCB rules) before
calling a board done — see `docs/_source/kicad.md` for the CLI.
