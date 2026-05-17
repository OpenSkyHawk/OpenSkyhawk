# Hardware Standards

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
