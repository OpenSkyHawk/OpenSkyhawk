# Hardware Standards

## Screws

| Screw | Use |
|---|---|
| M2 | PCB mounts, small standoffs |
| M3 | Placards, light rings, small brackets |
| M4 | Instrument bezels, gauge mounts — clearance Ø4.3–4.5 mm |
| M5 | Panel-to-subpanel, corner mounts — clearance Ø5.3–5.5 mm |

## Connectors

| Series | Pitch | Use |
|---|---|---|
| Molex MicroFit 3.0 | 3.0 mm | Inter-board harnesses (MCU ↔ breakout panels) |
| JST-XH | 2.54 mm | Switch/signal harnesses (panel-mounted controls → PCB) |

**Minimum pitch: 2.54 mm.** Nothing smaller is used anywhere in the build.

Standard inter-board harness (6-pin MicroFit 3.0):

| Pin | Signal |
|---|---|
| 1 | SDA |
| 2 | SCL |
| 3 | GND |
| 4 | GND |
| 5 | 12 V (LED backlight, PWM) |
| 6 | 3.3 V (chip power) |

Breakout boards with analog outputs may use a 7-pin variant (pin 7 = analog signal) routed to the STM32 ADC on the MCU board.

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
