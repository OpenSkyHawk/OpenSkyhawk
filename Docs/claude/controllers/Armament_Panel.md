# Armament Panel

**Location:** Center Console, main armament section  
**Controller:** Center_Armament — controls live directly on the `Armament_MCU` board (host panel, not a breakout)  
**Repo:** `PCB/Center_Console/Center_Armament/Armament_MCU/`

## Controls Inventory

Switch types, GPIO allocation, and wiring TBD — future plan. DCS-BIOS IDs confirmed from `Center_Armament.md`.

| DCS-BIOS ID | Control |
|---|---|
| 700 | Emergency Selector |
| 701 | Guns Ready |
| 702 | Arm Nose & Tail |
| 703 | Station 1 |
| 704 | Station 2 |
| 705 | Station 3 |
| 706 | Station 4 |
| 707 | Station 5 |
| 708 | Bombing Mode |
| 709 | Master |
| 710 | Cabin Pressure |

## I²C Device

| Address | Device | Notes |
|---|---|---|
| 0x20 | MCP23017 #1 | A0=A1=A2=LOW; switch inputs |
| 0x21 | MCP23017 #2 (optional) | A0=HIGH, A1=A2=LOW; lamp/output expansion |

## STM32 Analog Inputs (on Armament_MCU)

| STM32 Pin | Input | Type |
|---|---|---|
| PA0 | EMER SEL rotary | Resistor ladder |
| PA1 | MODE SEL (bombing mode) | Resistor ladder |

## Notes

- This panel's circuits are on the Armament_MCU board directly — no separate harness connector.
- I²C pull-ups (4.7 kΩ on SDA and SCL) are placed here, on the MCU board — **one set only**, not on breakouts.
- Cabin Pressure gauge driven by X27.589 Switec stepper (shaft-through-PCB), via DRV8835 on same board.

## Dimensions

TBD from Fusion 360 model.
