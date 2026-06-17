# Center_Armament Controller

**Location:** Center Console  
**Panels:** Armament Panel (host), AWRS Panel (breakout), Misc Switch Panel (breakout)  
**Repo:** `Firmware/Panels/Center_Armament/`, `PCB/Center_Console/Center_Armament/`

## MCU Board (Armament_MCU)

| Component | Details |
|---|---|
| MCU | STM32F103C8T6 (LQFP48, 64 KB flash, 20 KB RAM) |
| CAN transceiver | SN65HVD230 (SOIC-8, 3.3V logic) |
| CAN node ID | TBD |
| Voltage regulation | 5V from main bus (PC ATX PSU); AMS1117-3.3 (SOT-223) locally for 3.3V; no AP63205WU on this board |
| Stepper | X27.589 — Cabin Pressure gauge, shaft-through-PCB; driver: DRV8833PW (HTSSOP-16) |

## I²C Devices

| Address | Device | Notes |
|---|---|---|
| 0x22 | MCP23017 | Misc Switch Panel breakout — 12 digital inputs (A1=HIGH, A0=A2=LOW) |
| 0x48 | ADS1115 | AWRS Panel breakout — QTY, DROP INTVL, MODE SEL (ADDR tied to GND) |

Armament Panel switches (GUNS_READY, ARM_NOSE_TAIL, STATION_1–5, MASTER_ARMED) connect directly to STM32 GPIO via J_PANEL — no MCP23017 on the MCU board.

## STM32 Pin Assignments (Armament_MCU)

| STM32 Pin | Net | Via | Type |
|---|---|---|---|
| PA2 | MISSILE_VOL pot (726) | J2 pin 6 | Analog (pot wiper) |
| PA6 | PWM_PANEL_LED | — | TIM3 CH1 PWM — Misc Switch Panel LED zone |
| PA7 | PWM_GAUGE_LED | — | TIM3 CH2 PWM — gauge backlight zone |
| PB0 | SHRIKE_VOL_INT | J2 pin 8 | EXTI0 — MCP23017 INTB (Port B / Shrike only) |
| PB1 | MISC_SWITCH_INT | J2 pin 7 | EXTI1 — MCP23017 INTA (Port A / buttons, toggles, BDHI) |
| PB2 | STEPPER_A1 | — | DRV8833PW coil A OUT1 |
| PB3 | STEPPER_A2 | — | DRV8833PW coil A OUT2 |
| PB4 | STEPPER_B1 | — | DRV8833PW coil B OUT1 |
| PB5 | STEPPER_B2 | — | DRV8833PW coil B OUT2 |
| PB6 | SCL | J1/J2 | I2C1 clock |
| PB7 | SDA | J1/J2 | I2C1 data |
| PB12 | NSLEEP | — | DRV8833PW ~SLEEP (HIGH from setup()) |
| PB14 | STATUS_LED_RED | — | STM32Board red status LED — active HIGH |
| PB15 | STATUS_LED_GRN | — | STM32Board green status LED — active HIGH |

**Reserved / not used on this board:** PB10/PB11 (I2C2 — kept free per firmware contract).

**Deferred:** PA0 (EMER SEL), PA1 (MODE SEL), and all Armament Panel direct switches (GUNS_READY, ARM_NOSE_TAIL, STATION_1–5, MASTER_ARMED) — not implemented in this revision. Panel inputs not yet fully researched. J_PANEL connector omitted.

## Harness Connectors

### J1 → AWRS_Panel — 6-pin JST-XH

| Pin | Signal |
|---|---|
| 1 | SDA |
| 2 | SCL |
| 3 | GND |
| 4 | GND |
| 5 | +3.3V (chip power) |
| 6 | spare |

### J2 → Misc_Switch_Panel — 8-pin JST-XH

Pinout confirmed from Misc_Switch_Panel.kicad_sch (schematic is source of truth). 33Ω series dampers on SDA/SCL are on the Armament_MCU PCB side. 100Ω series resistors on interrupt lines (R20/R21) are on the Misc_Switch_Panel PCB side — do not add duplicates here.

| Pin | Signal | Notes |
|---|---|---|
| 1 | SDA | 33Ω damper (R_SDA_J2) on Armament_MCU PCB |
| 2 | SCL | 33Ω damper (R_SCL_J2) on Armament_MCU PCB |
| 3 | GND | |
| 4 | +3.3V | |
| 5 | NC | spare |
| 6 | MISSILE_VOL | Pot wiper → 1kΩ + 100nF → STM32 PA2 |
| 7 | MISC_SWITCH_INT | MCP23017 @ 0x22 INTA; 100Ω R21 already on Misc_Switch_Panel |
| 8 | SHRIKE_VOL_INT | MCP23017 @ 0x22 INTB; 100Ω R20 already on Misc_Switch_Panel |

### J_LED_MISC → Misc_Switch_Panel LED power — 2-pin Mini-Fit Jr

| Pin | Signal |
|---|---|
| 1 | BACKLIGHT_SW_RETURN (MOSFET drain) |
| 2 | +12V_BACKLIGHT |

LED zone MOSFET (IRLML2502 N-ch, low-side) lives on this board. Gate driven directly by STM32 PWM (3.3V) through 100Ω series resistor; 100kΩ pull-down to GND ensures LEDs off during reset. Drain → BACKLIGHT_SW_RETURN; source → GND. LED strings: +12V_BACKLIGHT → 120Ω resistor → 5× LEDs series → BACKLIGHT_SW_RETURN.

## Armament_MCU Schematic — Design Notes

See hardware-standards.md "Standard Circuit Blocks" for reference circuits.

| Item | Detail |
|---|---|
| LED zone switch | IRLML2502 N-ch low-side per zone. Gate ← 100Ω series ← STM32 PWM (3.3V). 100kΩ pull-down gate → GND (ensures LEDs off during STM32 reset). Drain → BACKLIGHT_SW_RETURN; source → GND. |
| I2C pull-ups | 4.7 kΩ on SDA and SCL — **one set only**, on MCU_CAN sheet, not on breakouts |
| I2C series dampers | 33 Ω on SDA/SCL at each harness connector (J1, J2) — on MCU_CAN sheet |
| Interrupt lines J2 | R20/R21 (100Ω) already on Misc_Switch_Panel PCB — **do not add duplicates** on J2 pins 7–8 |
| Interrupt STM32 pins | MISC_SWITCH_INT → PB1 (EXTI1); SHRIKE_VOL_INT → PB0 (EXTI0) |
| ADC filter — MISSILE_VOL | 1 kΩ series + 100 nF to GND at STM32 PA2 (J2 pin 6 → 1 kΩ → PA2 → 100 nF → GND) |
| ADC filter — EMER SEL | 1 kΩ + 100 nF at PA0 (resistor ladder pads, no ladder wired in this rev) |
| ADC filter — MODE SEL | 1 kΩ + 100 nF at PA1 (resistor ladder pads, no ladder wired in this rev) |
| Status LEDs | PB14 → R → D_RED → GND; PB15 → R → D_GRN → GND. Active HIGH. R23/R24 = 3.3kΩ → ~0.4mA (intentionally dim — cockpit use). |
| Stepper pins | DRV8833PW: A1=PB2, A2=PB3, B1=PB4, B2=PB5; ~SLEEP=PB12 |

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
