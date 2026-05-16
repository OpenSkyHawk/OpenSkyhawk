# Center_Armament Controller

**Location:** Center Console  
**Panels:** Armament Panel (host), AWRS Panel (breakout), Misc Switch Panel (breakout)  
**Repo:** `Firmware/Center_Armament/`, `PCB/Center_Console/Center_Armament/`

## MCU Board (Armament_MCU)

| Component | Details |
|---|---|
| MCU | STM32 (TBD variant) |
| CAN node ID | TBD |
| CAN transceiver | TBD |
| Voltage regulation | 5 V + 3.3 V (TBD regulators) |
| Stepper | X27.589 — Cabin Pressure gauge, shaft-through-PCB |

## I²C Devices

| Address | Device | Notes |
|---|---|---|
| TBD | MCP23017 #1 | Switch inputs |
| TBD | MCP23017 #2 (optional) | Lamp/output expansion |
| TBD | ADS1115 (AWRS breakout) | AWRS QTY, DROP INTVL, MODE SEL |
| TBD | MCP23017 (Misc Switch breakout) | Toggle inputs |

## ADC Inputs (on Armament_MCU board)

| Channel | Input | Type |
|---|---|---|
| TBD | EMER SEL rotary | Resistor ladder |
| TBD | MODE SEL lever (bombing mode) | Resistor ladder |

## Harness Connectors

| Connector | To | Pins |
|---|---|---|
| J1 | AWRS_Panel | 6-pin MicroFit 3.0 (SDA, SCL, GND, GND, 12V, 3.3V) |
| J2 | Misc_Switch_Panel | 6-pin MicroFit 3.0 (SDA, SCL, GND, GND, 12V, 3.3V) |

## DCS-BIOS Mappings (Armament Panel)

| ID | Control |
|---|---|
| 700 | Emergency Selector |
| 701 | Guns Ready |
| 702 | Arm Nose & Tail |
| 703–707 | Stations 1–5 |
| 708 | Bombing Mode |
| 709 | Master |
| 710 | Cabin Pressure |
