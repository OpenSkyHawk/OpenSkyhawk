# PCB Design Rules

Every OpenSkyhawk board is fabbed at **JLCPCB, standard 2-layer service**. These rules are
pre-loaded into each new KiCad project by the `/new-kicad-project` scaffold — you shouldn't have
to set them by hand. This page is the reference.

## Minimum design rules

| Constraint | Value | Notes |
|------------|-------|-------|
| Min trace width | **0.2 mm** | JLCPCB floor is 0.127 mm; bumped so DRC violations are meaningful |
| Min spacing / clearance | **0.2 mm** | all IC packages in this build route fine at 0.2 mm |
| Min via drill | **0.3 mm** | smaller needs advanced service |
| Min via annular ring | **0.13 mm** | → 0.56 mm min pad |
| Min PTH drill-to-copper | **0.254 mm** | |
| Min hole-to-hole | **0.5 mm** | edge to edge |
| Min copper-to-edge | **0.3 mm** | |
| Min silk stroke | **0.153 mm** | use 0.15 mm in KiCad |
| Min silk text height | **0.8 mm** | 1.0 mm recommended |
| Board outline width | **0.05 mm** | Edge.Cuts |
| Copper weight | **1 oz** | standard |

!!! note "Routing between IC pads"
    LQFP48 and HTSSOP pad gaps are too narrow to route between at 0.2 mm clearance regardless —
    use pad-end escape routing (best practice anyway). SOIC-8/28 (1.27 mm pitch) routes
    comfortably between pads.

## Net classes

Four classes are pre-loaded; pattern-matched nets are assigned automatically.

| Class | Track | Via | Assignment |
|-------|-------|-----|------------|
| `Default` | 0.2 mm | 0.3/0.6 mm | everything not below |
| `LED_Trunk` | 0.5 mm | 0.4/0.8 mm | auto: `+12V_BACKLIGHT`, `BACKLIGHT_SW_RETURN` |
| `LED_String` | 0.3 mm | 0.3/0.6 mm | **manual** — per-string traces (resistor → 5 LEDs) |
| `CAN` | 0.2 mm | 0.3/0.6 mm | auto: `CANH`, `CANL` — route as a diff pair (0.2 mm gap) |

`LED_String` nets are auto-named by KiCad, so pattern matching can't reach them — pick 0.3 mm
from the predefined widths when routing those traces.

## Predefined track widths

| Width | Use |
|-------|-----|
| 0.2 mm | signal (default) |
| 0.3 mm | LED string feeds (~0.75 A) |
| 0.5 mm | 5 V / 3.3 V power (~1.2 A) |
| 1.0 mm | 12 V main power (~2.5 A) |

## Placement

- **Front face: LEDs only.** Back face: ICs, passives, connectors, MOSFETs.
- Connectors: through-hole, vertical, accessible from the panel side.
- PTH parts extend through to the front — keep silk and copper clear of LED pads.

## Layer stackup

Standard 2-layer: F.Cu / B.Cu, F/B.Paste, F.Silkscreen (B.Silkscreen kept clear for
inspection), F/B.Courtyard, Edge.Cuts at 0.05 mm.

## Board power budget

Logic + LED boards target **≤ 500 mA at 12 V input**. Full breakdown, plus the actuator-board
caveat, is on the [Power Architecture](../architecture/power.md) page.

## JLCPCB ordering (cost-optimised)

1 oz outer copper, tented vias, 0.3 mm min via, ±0.2 mm outline tolerance, flying-probe test,
remove JLCPCB mark, no gold fingers / castellation / edge plating. Skip manual file
confirmation to cut lead time.

## Custom DRC file

`PCB/Libraries/design-rules/jlcpcb-standard.kicad_dru` holds these as enforceable rules; it's
copied into each new project by the scaffold. To add to an existing project: **Board Setup →
Design Rules → Custom Rules** and paste it in.
