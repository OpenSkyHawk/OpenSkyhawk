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
| TBD | MCP23017 (Misc Switch breakout) | 12 digital inputs (switches 720–727); see Misc_Switch_Panel.md |

## ADC Inputs (on Armament_MCU board)

| Channel | Input | Type |
|---|---|---|
| TBD | EMER SEL rotary | Resistor ladder |
| TBD | MODE SEL lever (bombing mode) | Resistor ladder |
| TBD | MISSILE_VOL pot (726, via J2 pin 7) | Analog (pot wiper, −1 to +1 V range) |

## Harness Connectors

| Connector | To | Pins |
|---|---|---|
| J1 | AWRS_Panel | 6-pin JST-XH (SDA, SCL, GND, GND, 12V, 3.3V) |
| J2 | Misc_Switch_Panel | 8-pin JST-XH (SDA, SCL, GND, GND, 12V, 3.3V, ANALOG, spare) |

J2 pin 7 carries the MISSILE_VOL pot wiper from the Misc Switch Panel back to the STM32 ADC.

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
