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
| 5 | 12 V (LED backlight, PWM) |
| 6 | 3.3 V (chip power) |

Breakout boards with analog outputs use an 8-pin variant (pins 7 = analog signal, pin 8 = spare).

## Switches & Controls

- Toggle switches: 12 mm (standard), ~6 mm (ECM modules)

## LED Backlighting

- Type: 5050 SMD, red
- Placement: PCB front face; all other components on back face
- Grouping: arrays of 5 LEDs
- Drive current: ~30 mA per LED (60 mA max rated)
- Power: 12 V PWM line from inter-board harness
- PCB trace width: 0.3 mm minimum for LED array feeds

## Gauges

- LOX gauge: 2-5/8″ (~67 mm total)
- Radar Altimeter gauge: 3-1/8″ (~100 mm with bezel)
- Cabin Pressure gauge: driven by X27.589 Switec stepper (shaft-through-PCB mount)
