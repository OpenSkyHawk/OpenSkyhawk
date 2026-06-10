# Shared Hierarchical Sheet Templates

KiCad `.kicad_sch` files in this directory are reusable standard circuit blocks for OpenSkyhawk boards.

## How to use

In KiCad Schematic Editor: **Place → Add Sheet**, set filename to the path of the template here, then assign hierarchical labels to match your net names.

Alternatively, open the template file directly and copy the relevant symbols/wiring into your sheet.

## Available templates

| File | Circuit block | Reference doc |
|------|--------------|---------------|
| (none yet) | | |

Templates are added here as each standard block is first drawn and validated. See `docs/claude/hardware-standards.md` **Standard Circuit Blocks** for the target list:

- `power_rail.kicad_sch` — AP63205WU (12V→5V) + AMS1117-3.3 (5V→3.3V)
- `mcp23017.kicad_sch` — MCP23017 with decoupling (100nF+10µF), address resistors, I2C series resistors (33Ω), interrupt protection (100Ω)
- `led_zone.kicad_sch` — IRLML2502 N-ch MOSFET + 2-pin Mini-Fit Jr connector, gate driven by STM32 PWM
- `adc_filter.kicad_sch` — 1kΩ series + 100nF to GND per ADC input
- `can_transceiver.kicad_sch` — SN65HVD230 with 120Ω bus termination option

## Adding a new template

1. Draw and validate the subcircuit in a project schematic
2. Save a copy of the sheet file here with a descriptive name
3. Update the table above
4. Commit — all future `/new-kicad-project` scaffolds will list it as available
