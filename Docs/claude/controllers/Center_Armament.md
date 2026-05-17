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
| PA2 | MISSILE_VOL pot (726, via J2 pin 7) | Analog (pot wiper, −1 to +1 V range) |

## Harness Connectors

| Connector | To | Pins |
|---|---|---|
| J1 | AWRS_Panel | 6-pin JST-XH (SDA, SCL, GND, GND, 12V switched, 3.3V) |
| J2 | Misc_Switch_Panel | 8-pin JST-XH (SDA, SCL, GND, GND, 12V switched, 3.3V, ANALOG, spare) |

J2 pin 7 carries the MISSILE_VOL pot wiper from the Misc Switch Panel back to the STM32 ADC (PA2).

Harness pin 5 (12V) is the MOSFET-switched output — the IRLML2502 LED zone MOSFET lives on this board and its drain drives pin 5 of each harness.

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
