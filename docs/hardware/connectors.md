# Connector & Harness Guide

Two connector families cover the whole cockpit: **Molex Mini-Fit Jr** for power and the main
bus, **JST-XH** for signal and switch wiring. Nothing smaller than **2.54 mm pitch** is used
anywhere, and wiring is **24 AWG throughout**.

## Molex Mini-Fit Jr (4.2 mm) — main bus + LED power

Carries the inter-board bus (CAN + power) and the LED backlight power on every board.
Through-hole, vertical, polarized (one insertion orientation). Crimp tool: JRready ST6490-ACT.

### Main bus — 2×4 (8-pin)

`Connector_Molex:Molex_Mini-Fit_Jr_5566-08A2_2x04_P4.20mm_Vertical`. **Two identical connectors
per MCU board** (`J_BUS_IN` + `J_BUS_OUT`) — the bus passes straight through. (5566 = vertical
PCB header; the mating cable side is the 5557 receptacle housing — see the wiring/harness notes.)

| Pin | Signal | Notes |
|-----|--------|-------|
| 1 | +12V | always-on 12 V from PSU |
| 2 | +12V | parallel with pin 1 — lowers connector resistance for LED current |
| 3 | +5V | from PSU; 3.3 V generated locally |
| 4 | GND | |
| 5 | CANH | differential pair with pin 6 |
| 6 | CANL | differential pair with pin 5 |
| 7 | GND | |
| 8 | GND | |

CANH/CANL share a row (pins 5/6) for clean differential routing.

### LED power — 2-pin (1×2)

`Connector_Molex:Molex_Mini-Fit_Jr_5566-02A2_2x01_P4.20mm_Vertical`. One per lighting zone,
separate from the signal harness:

| Pin | Signal |
|-----|--------|
| 1 | `BACKLIGHT_SW_RETURN` (MOSFET drain — near GND when LEDs on) |
| 2 | `+12V_BACKLIGHT` (always-on 12 V to the LED string tops) |

## JST-XH (2.54 mm) — signal + switch harnesses

Intra-group wiring — MCU ↔ breakout (I²C, interrupts, analog) and switch harnesses.
Through-hole, single-row, vertical, polarized. Rated 3 A/pin. Crimp tool: Engineer PA-09.
Standard sizes **4 / 6 / 8 pin** — pick by pin count, leave no pins empty.

Standard 6-pin signal harness:

| Pin | Signal |
|-----|--------|
| 1 | SDA |
| 2 | SCL |
| 3 | GND |
| 4 | GND |
| 5 | 3.3 V (chip power) |
| 6 | spare |

Breakouts with analog outputs or interrupts use the 8-pin variant with extra signals on pins
6–8. Switches share a common GND within each connector group (one GND pin per connector).

!!! note "LED power is never on the signal harness"
    Backlight power always rides its own 2-pin Mini-Fit Jr connector — never mixed into the
    JST-XH signal harness.
