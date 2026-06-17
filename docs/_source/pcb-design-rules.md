# PCB Design Rules

Fabricator: **JLCPCB**, standard 2-layer service.

All KiCad projects are scaffolded with these constraints pre-loaded via `/new-kicad-project`.
The custom DRC rule file is at `PCB/Libraries/design-rules/jlcpcb-standard.kicad_dru`.

---

## Minimum Design Rules (JLCPCB Standard 2-Layer)

| Constraint | Value | Notes |
|---|---|---|
| Min trace width | **0.2 mm** | JLCPCB floor is 0.127mm; bumped to match Default class — DRC violations are always meaningful |
| Min trace spacing / clearance | **0.2 mm** | JLCPCB floor is 0.127mm; bumped — all IC packages in this build are compatible (see note below) |
| Min via drill | **0.3 mm** | Smaller (0.2mm) requires advanced service |
| Min via annular ring | **0.13 mm** | Pad = drill + 2 × ring → 0.3 + 0.26 = **0.56 mm** min pad |
| Min PTH drill-to-copper | **0.254 mm** | Clearance from PTH edge to nearest copper |
| Min hole-to-hole (edge to edge) | **0.5 mm** | Between any two drilled holes |
| Min copper-to-board-edge | **0.3 mm** | Copper must stay ≥ 0.3mm inside board outline |
| Min silkscreen stroke | **0.153 mm** | Use 0.15mm in KiCad (rounds up at fabrication) |
| Min silkscreen text height | **0.8 mm** | 1.0mm recommended for legibility |
| Board outline line width | **0.05 mm** | Edge.Cuts layer — thinner = more accurate |
| Copper weight | **1 oz** | Standard; heavier copper adds cost |

---

## Predefined Sizes in KiCad Template

These are pre-loaded in `template.kicad_pro` and appear in the "Interactive Router Settings" dropdowns.

### Track widths

| Width | Use |
|---|---|
| 0.2 mm | Signal traces (default) — comfortably above 0.127mm minimum |
| 0.3 mm | LED string feeds — required per hardware-standards.md; ~0.75A capacity |
| 0.5 mm | 5V / 3.3V power traces — ~1.2A capacity |
| 1.0 mm | 12V main power — ~2.5A capacity; covers full 1.8A LED load with margin |

Current capacity estimates at 1oz copper, 10°C temperature rise (IPC-2221B).

**Package compatibility note:** LQFP48 (STM32) and HTSSOP-16 (DRV8833PW) have pad gaps of 0.28mm and 0.35mm respectively — too narrow to route between pads at 0.2mm clearance regardless. Use pad-end escape routing for both (best practice anyway). SOIC-8/28 (1.27mm pitch, 0.67mm gap) routes comfortably between pads at 0.2mm clearance.

### Via sizes

| Drill | Pad | Use |
|---|---|---|
| 0.3 mm | 0.6 mm | Standard via — signal and low-current power; minimum JLCPCB spec |
| 0.4 mm | 0.8 mm | Power / stitching vias — 12V and GND planes |

---

## Default KiCad Text Settings (Silkscreen)

| Setting | Value |
|---|---|
| Text height | 1.0 mm |
| Text stroke | 0.15 mm |
| Fab / copper text height | 1.0 mm |
| Fab / copper text stroke | 0.15 mm |

---

## JLCPCB Ordering Settings (Cost-Optimised)

These settings minimise cost on every order. Select them in the JLCPCB order page under **High-Spec Options**.

| Option | Setting | Notes |
|---|---|---|
| Outer Copper Weight | **1 oz** | Standard; sufficient for all current loads in this build |
| Via Covering | **Tented** | Soldermask over via pads; cheapest; no "via-in-pad" issues |
| Via Plating Method | **Not Specified** | No preference — cheapest default |
| Min Via Hole / Diameter | **0.3mm / (0.4/0.45mm)** | Matches template minimum; standard tier |
| Board Outline Tolerance | **±0.2mm (Regular)** | Sufficient for panel fit; precision adds cost |
| Confirm Production File | **No** | Skip manual review; reduces lead time |
| Mark on PCB | **Remove Mark** | No JLCPCB branding mark on board |
| Electrical Test | **Flying Probe Fully Test** | Included at standard tier; catches open/short faults |
| Gold Fingers | **No** | |
| Castellated Holes | **No** | |
| Edge Plating | **No** | |
| Blind Slots | **No** | |
| UL Marking | **No** | |
| Humidity Indicator Card | **No** | |

---

## KiCad Layer Stackup

Standard 2-layer assignment:

| Layer | Use |
|---|---|
| F.Cu | Component-side copper |
| B.Cu | Back copper |
| F.Paste | Front solder paste (SMD) |
| B.Paste | Back solder paste (SMD) |
| F.Silkscreen | Component labels, reference designators |
| B.Silkscreen | (optional, keep clear for inspection) |
| F.Courtyard | Component keep-out areas |
| B.Courtyard | Component keep-out areas (back) |
| Edge.Cuts | Board outline — 0.05mm line width |

---

## Component Placement Rules

- **Front face:** LEDs only
- **Back face:** all other components (ICs, passives, connectors, MOSFETs)
- Connectors: through-hole, vertical — accessible from the panel side
- PTH components extend through to front face — ensure no silk or copper conflict with LED pads

---

## Net Classes

Four net classes are pre-loaded in `template.kicad_pro`. Pattern-matched nets are assigned automatically; `LED_String` requires manual assignment in the PCB editor.

| Class | Track | Clearance | Via drill/pad | Assignment |
|---|---|---|---|---|
| `Default` | 0.2 mm | 0.127 mm | 0.3/0.6 mm | All nets not listed below |
| `LED_Trunk` | 0.5 mm | 0.127 mm | 0.4/0.8 mm | Auto: `+12V_BACKLIGHT`, `BACKLIGHT_SW_RETURN` |
| `LED_String` | 0.3 mm | 0.127 mm | 0.3/0.6 mm | Manual — per-string traces (resistor → 5× LEDs) |
| `CAN` | 0.2 mm | 0.2 mm | 0.3/0.6 mm | Auto: `CANH`, `CANL`; route as diff pair (0.2 mm gap) |

**LED_Trunk sizing rationale:** zone trunk carries all strings in parallel. At 18 mA/string, 0.5 mm handles up to 30 strings (540 mA) with 2.2× margin at 1oz copper.

**LED_String manual assignment:** per-string nets between resistor and LED chain are unnamed in KiCad (auto-named `Net-(Rx-Pad2)`); pattern matching cannot reach them. Select 0.3 mm from the predefined widths dropdown when routing these traces.

### Deferred — Power net classes

Power net classes (`Power_12V`, `Power_LV`) are **not yet defined**. They depend on actuator architecture (servos, solenoids, large steppers) that is not yet finalised. Revisit when the first actuator board is designed.

**Established boundary:** logic + LED boards must stay ≤ 500 mA total 12V input. Actuator boards are sized to their specific loads with no general limit — those boards require separate design review (see Actuator Boards section below).

---

## Board Power Budget

### Logic + LED boards (MCU and breakout)

Target: **≤ 500 mA at 12V input per board.**

| Rail | Typical load | Max expected |
|---|---|---|
| 12V → LEDs | 54–180 mA (3–10 strings) | ~360 mA (20 strings, large panel) |
| 12V → AP63205 input | ~100 mA | ~150 mA |
| **Total 12V per board** | ~160–280 mA | ~510 mA (large panel edge case) |
| 5V (DRV8833 stepper VM) | 15–30 mA | 50 mA |
| 3.3V (STM32 + MCP23017 + CAN) | ~125 mA | ~175 mA |

**System total (full cockpit ~20 boards):** ~2.3 A at 12V. PC ATX 500–600W supply provides 35–40A on 12V — <7% utilisation.

### Actuator boards (solenoids, servos, large steppers)

**Not yet designed.** Each actuator board requires its own power budget analysis before PCB work begins. Minimum considerations:

- [ ] Solenoid / electromagnetic switch: confirm coil resistance, voltage, duty cycle, and whether flyback protection is needed (it almost certainly is — add TVS or flyback diode)
- [ ] Servo: confirm supply voltage (5V or dedicated), stall current, and whether a separate high-current supply rail is needed
- [ ] Large stepper (NEMA 14/17): confirm driver IC (the DRV8833 gauge driver is **not** suitable — max ~1.5A/phase; use DRV8825, TMC2209, or similar), current per phase, and microstepping requirements
- [ ] Decide: high-current actuator load on same board as logic, or dedicated driver board with separate power connector?

---

## Custom DRC Rules

`PCB/Libraries/design-rules/jlcpcb-standard.kicad_dru` is copied into each new project by `/new-kicad-project`.

To activate in an existing project: open the PCB editor → **File → Board Setup → Design Rules → Custom Rules** → paste the file contents (or browse to the file).
