# Hardware Standards

## Component Package Rules

**Constraint: all packages must be visually inspectable after reflow.** T962 reflow oven is available; the limitation is inspection, not soldering.

| Acceptable | Not acceptable |
|------------|---------------|
| SOIC, SSOP, TSSOP, HTSSOP | QFN, DFN, WSON |
| LQFP | BGA, LGA |
| SOT-23, SOT-223 | Any fully-bottom-terminated package |
| Through-hole | |

HTSSOP (exposed thermal pad on underside) is acceptable — side leads are the critical joints; the GND pad is verified by continuity check.

## MCU

**Selected: STM32F103CBT6** — LQFP48, 128 KB flash, 20 KB RAM.

- Requires **external 8 MHz crystal** for reliable CAN bus timing (internal RC oscillator is not accurate enough for CAN bit-rate lock)
- **PA11/PA12** are shared between USB and CAN — the two peripherals cannot be used simultaneously; pick one at firmware init
- CAN bus is the primary inter-board protocol; USB used only for initial flashing/debug
- Use the bare die (not Blue Pill module) on MCU boards — fewer passive conflicts, smaller footprint

## Power Supply

**Architecture: 12 V → AP63205 (buck) → 5 V → AMS1117-3.3 (LDO) → 3.3 V**

| Stage | Part | Package | Notes |
|-------|------|---------|-------|
| 12 V → 5 V | AP63205WU | SOT-23-6 | Switching buck; typical BOM: C_in 10 µF, C_bypass 100 nF, L 4.7 µH, C_out 2×22 µF |
| 5 V → 3.3 V | AMS1117-3.3 | SOT-223 | LDO; 5→3.3 V drop (1.7 V) is acceptable; 12→5 V via LDO would dissipate ~1.4 W — not acceptable |
| 5 V rail | also feeds | — | DRV8835 VM (stepper driver motor supply) |

LDO is correct for the 5 V → 3.3 V stage only. Never use a linear regulator for 12 V → 5 V.

## Stepper Driver

**Selected: DRV8835 (TI)** — dual H-bridge, HTSSOP-16.

- Drives one X27.589 Switec stepper (bipolar, ~600 steps/315°, 180–300 Ω coils, ~15–30 mA at 5 V)
- VM supply: 5 V; VCC logic: 3.3 V
- **nSLEEP pin**: held LOW by MCU at power-on, driven HIGH only after DCS-BIOS sim connection is established — prevents startup needle twitch
- No current-regulation passives needed; X27.589 coil resistance limits current naturally at 5 V
- Firmware: use **SwitecX25** library (handles homing/reset); AccelStepper does not implement homing

## Screws

| Screw | Use |
|---|---|
| M2 | PCB mounts, small standoffs |
| M3 | Placards, light rings, small brackets |
| M4 | Instrument bezels, gauge mounts — clearance Ø4.3–4.5 mm |
| M5 | Panel-to-subpanel, corner mounts — clearance Ø5.3–5.5 mm |

## Connectors

| Series | Pitch | Use | Tooling |
|---|---|---|---|
| Molex Mini-Fit Jr | 4.2 mm | Main bus — CAN bus + power between controller groups | JRready ST6490-ACT |
| JST-XH | 2.54 mm | Everything else — MCU ↔ breakout harnesses + switch/signal harnesses | Engineer PA-09 |

**Minimum pitch: 2.54 mm.** Nothing smaller is used anywhere in the build.

**Wire gauge: 24 AWG throughout.**

### Molex Mini-Fit Jr (main bus)

- **PCB footprint:** Through-hole, dual-row, vertical
- Carries CAN bus signals and power distribution between controller groups
- Polarized housing — one insertion orientation only

### JST-XH (intra-group harnesses + switch wiring)

- **PCB footprint:** Through-hole, single-row, vertical
- **Standard sizes:** 4-pin, 6-pin, 8-pin — choose by pin count, leave no pins empty
- Rated 3A per pin — sufficient for 12V LED lines and all signal/power within a controller group
- Polarized housing — one insertion orientation only
- Switches share a common GND within each connector group; one GND pin per connector

Standard intra-group harness (6-pin JST-XH):

| Pin | Signal |
|---|---|
| 1 | SDA |
| 2 | SCL |
| 3 | GND |
| 4 | GND |
| 5 | 12 V switched (LED backlight — MOSFET output from MCU board) |
| 6 | 3.3 V (chip power) |

Breakout boards with analog outputs use an 8-pin variant (pins 7 = analog signal, pin 8 = spare).

## Switches & Controls

- Toggle switches: 12 mm (standard), ~6 mm (ECM modules)

## CAN Transceiver

**Selected: SN65HVD230** — SOIC-8, 3.3V logic.

- One per MCU board; connects STM32F103 CAN controller (PA11/PA12) to the physical bus
- Runs directly from 3.3V rail — no level shifter required
- CANH/CANL route to Molex Mini-Fit Jr main bus connector
- **Bus termination:** 120Ω resistor across CANH/CANL on each end node; omit on intermediate nodes
- Compatible 3.3V clones (e.g. VP230) acceptable for JLCPCB assembly

## LED Backlighting

**Architecture: 5-LED series strings, MOSFET-switched per zone, resistor current limiting.**

Confirmed from bench testing. Estimated ~500 LEDs total across full cockpit (~100 strings of 5).

### String design

- 5 × 5050 SMD red LEDs in series per string
- Measured Vf: 1.95–2.1 V per LED → ~10 V total per string at 12 V supply → ~2 V headroom for resistor
- One current-limiting resistor per string (back face of PCB)

### Resistor values (bench-tested)

| Resistor | Measured current | Use |
|---|---|---|
| 47Ω | 42–55 mA | **Rejected** — resistor overheated |
| 100Ω | 19–23 mA | Bright panels / gauges |
| 120Ω | ~17–20 mA | **Balanced default** |
| 150Ω | 14–17 mA | Standard |
| 180Ω | ~11–13 mA | Low brightness / night-friendly |
| 200Ω | 10–12 mA | Dimmest |

**Default: 120Ω.** Choose at assembly time per zone; 100Ω for bright panels, 180Ω for dimmer zones.

**Resistor package:** 0805 minimum, 1206 preferred where board space allows. Dissipation ≈ 39 mW at 120Ω/18 mA — either package is thermally fine; 1206 is easier to hand-solder and rework.

### Zone dimming (MOSFET)

- One **IRLML2502** N-channel MOSFET per panel/lighting zone (SOT-23, 20V, 4A, Vgs(th) 0.3–0.7V)
- Gate driven by STM32 PWM output (3.3V fully enhances the IRLML2502)
- MOSFET switches the 12V LED rail; harness pin 5 carries the switched 12V output
- Resistors set per-string current at full on; PWM controls average brightness across the zone

### Other

- Placement: LEDs on PCB front face; resistors, MOSFETs, and all other components on back face
- PCB trace width: 0.3 mm minimum for LED string feeds
- 5-LED strings preferred over 3-LED: fewer parallel paths, lower wiring current (1.8A total at 120Ω vs ~3A for 3-LED strings across same ~500 LEDs)

## Gauges

- LOX gauge: 2-5/8″ (~67 mm total)
- Radar Altimeter gauge: 3-1/8″ (~100 mm with bezel)
- Cabin Pressure gauge: driven by X27.589 Switec stepper (shaft-through-PCB mount)
