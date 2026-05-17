# Center_Armament Controller

**Location:** Center Console  
**Panels:** Armament Panel (host), AWRS Panel (breakout), Misc Switch Panel (breakout)  
**Repo:** `Firmware/Center_Armament/`, `PCB/Center_Console/Center_Armament/`

## MCU Board (Armament_MCU)

| Component | Details |
|---|---|
| MCU | STM32F103CBT6 (LQFP48, 128 KB flash, 20 KB RAM) |
| CAN transceiver | SN65HVD230 (SOIC-8, 3.3V logic) |
| CAN node ID | TBD |
| Voltage regulation | AP63205WU (12V → 5V buck) + AMS1117-3.3 (5V → 3.3V LDO) |
| Stepper | X27.589 — Cabin Pressure gauge, shaft-through-PCB |

## I²C Devices

| Address | Device | Notes |
|---|---|---|
| 0x20 | MCP23017 #1 | Armament Panel switch inputs (A0=A1=A2=LOW) |
| 0x21 | MCP23017 #2 (optional) | Lamp/output expansion (A0=HIGH, A1=A2=LOW) |
| 0x22 | MCP23017 | Misc Switch Panel breakout — 12 digital inputs (A1=HIGH, A0=A2=LOW) |
| 0x48 | ADS1115 | AWRS Panel breakout — QTY, DROP INTVL, MODE SEL (ADDR tied to GND) |

## ADC Inputs (on Armament_MCU board)

| STM32 Pin | Input | Type |
|---|---|---|
| PA0 | EMER SEL rotary | Resistor ladder |
| PA1 | MODE SEL lever (bombing mode) | Resistor ladder |
| PA2 | MISSILE_VOL pot (726, via J2 pin 6) | Analog (pot wiper, −1 to +1 V range) |
| TBD | MISC_SWITCH_INT from Misc_Switch_Panel (via J2 pin 7) | EXTI — Port A changes (buttons, toggles, BDHI) |
| TBD | SHRIKE_VOL_INT from Misc_Switch_Panel (via J2 pin 8) | EXTI — Port B changes (Shrike only) |

## Harness Connectors

| Connector | To | Pins |
|---|---|---|
| J1 | AWRS_Panel | 6-pin JST-XH (SDA, SCL, GND, GND, 12V switched, 3.3V) |
| J2 | Misc_Switch_Panel | 8-pin JST-XH (SDA, SCL, GND, 3.3V, NC, MISSILE_VOL, MISC_SWITCH_INT, SHRIKE_VOL_INT) |
| J2_LED | Misc_Switch_Panel LED | 2-pin Mini-Fit Jr (+12V_BACKLIGHT, BACKLIGHT_SW_RETURN) |

J2 pin 6 carries the MISSILE_VOL pot wiper from the Misc Switch Panel back to the STM32 ADC (PA2).

LED zone MOSFET (IRLML2502 N-ch, low-side) lives on this board. Gate driven directly by STM32 PWM (3.3V). Drain → BACKLIGHT_SW_RETURN (J2_LED pin 1); source → GND. LED strings: +12V_BACKLIGHT (J2_LED pin 2) → 120Ω resistor → 5× LEDs series → BACKLIGHT_SW_RETURN. J2 pin 5 is NC.

## Armament_MCU Schematic — Pending Items

These must be addressed when the Armament_MCU schematic is started. See hardware-standards.md "Standard Circuit Blocks" for reference circuits.

| Item | Detail |
|---|---|
| LED zone switch | IRLML2502 N-ch low-side per zone. Gate ← STM32 PWM (3.3V direct, no driver needed). Drain → BACKLIGHT_SW_RETURN (J1_LED pin 1, J2_LED pin 1); source → GND. See standard circuit block. |
| I2C pull-ups | 4.7 kΩ on SDA and SCL — **one set only**, on this board, not on breakouts |
| I2C series resistors | 33 Ω on SDA/SCL at each harness connector (J1, J2) — between bus and connector pin |
| Interrupt protection | 100 Ω series on INTA/INTB at J2 pins 7–8 before leaving board |
| ADC filter — MISSILE_VOL | 1 kΩ series + 100 nF to GND at STM32 PA2 (J2 pin 6 → 1 kΩ → PA2, PA2 → 100 nF → GND) |
| ADC filter — EMER SEL | 1 kΩ + 100 nF at PA0 (resistor ladder input) |
| ADC filter — MODE SEL | 1 kΩ + 100 nF at PA1 (resistor ladder input) |
| Gate pull-up resistors | 100 kΩ from each IRLML6402 gate to +12V (default-off) |
| NPN gate driver resistor | 330 Ω from STM32 PWM pin to NPN base (per LED zone) |

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

## Misc Switch Panel — MCP23017 GPIO Allocation (0x22)

Port A handles buttons/toggles/BDHI (INTA interrupt). Port B handles Shrike only (INTB interrupt — clean isolation).

| MCP23017 Pin | Net Label     | Control                        | DCS-BIOS |
|---|---|---|---|
| PA0 | FUEL_EXT_BTN  | Show EXT Fuel (pushbutton)     | 720 |
| PA1 | MASTER_TEST   | Master Test (pushbutton)       | 723 |
| PA2 | RADAR_PROFILE | Radar Plan/Profile (2-pos)     | 721 |
| PA3 | RADAR_RANGE   | Radar Long/Short Range (2-pos) | 722 |
| PA4 | CONT_NORM     | CONT/NORM (spare)              | 727 (not in DCS) |
| PA5 | BDHI_1        | BDHI NAV PAC position          | 724 |
| PA6 | BDHI_3        | BDHI NAV CMPTR position        | 724 |
| PA7 | —             | Spare (no-connect)             | — |
| PB0 | SHRIKE_1      | Shrike pos 1                   | 725 |
| PB1 | SHRIKE_2      | Shrike pos 2                   | 725 |
| PB2 | SHRIKE_3      | Shrike pos 3                   | 725 |
| PB3 | SHRIKE_4      | Shrike pos 4                   | 725 |
| PB4 | SHRIKE_5      | Shrike pos 5                   | 725 |
| PB5–PB7 | —         | Spare (no-connect)             | — |

MISSILE_VOL pot wiper → J3 pin 2 → net `MISSILE_VOL` → J1 pin 7 → STM32 PA2. Not on MCP23017.

### Switch Connector Pinouts (on Misc_Switch_Panel PCB)

**J2 — 8-pin JST-XH — Switches:**

| Pin | Net |
|---|---|
| 1 | FUEL_EXT_BTN |
| 2 | MASTER_TEST |
| 3 | RADAR_PROFILE |
| 4 | RADAR_RANGE |
| 5 | CONT_NORM |
| 6 | BDHI_T1 |
| 7 | BDHI_T3 |
| 8 | GND |

**J3 — 8-pin JST-XH — Pot + Shrike:**

| Pin | Net |
|---|---|
| 1 | +3V3 |
| 2 | MISSILE_VOL (pot wiper) |
| 3 | SHRIKE_1 |
| 4 | SHRIKE_2 |
| 5 | SHRIKE_3 |
| 6 | SHRIKE_4 |
| 7 | SHRIKE_5 |
| 8 | GND |

## DCS-BIOS Mappings (Misc Switch Panel)

| DCS-BIOS ID | Point | Control |
|---|---|---|
| FUEL_EXT_BTN | 720 | Show EXT Fuel (pushbutton) |
| RADAR_PROFILE | 721 | Radar Plan/Profile (2-pos toggle) |
| RADAR_RANGE | 722 | Radar Long/Short Range (2-pos toggle) |
| MASTER_TEST | 723 | Master Test (pushbutton) |
| BDHI_MODE | 724 | BDHI mode — NAV PAC / TACAN / NAV CMPTR (3-pos toggle) |
| SHRIKE_SEL_KNB | 725 | Shrike Selector Knob (5-pos rotary; handler stub in mod) |
| MISSILE_VOL | 726 | Missile Volume Knob (pot, −1 to +1) |
| — | 727 | CONT/NORM (2-pos toggle; not in DCS — spare GPIO) |
