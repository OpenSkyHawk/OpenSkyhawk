# Misc Switch Panel

**Location:** Center Console, below LOX gauge  
**Controller:** Center_Armament (breakout via J2)  
**DCS section:** #36  
**Repo:** `PCB/Center_Console/Center_Armament/Misc_Switch_Panel/`  
**Sources:** `clickabledata.lua`, `A-4E-C.lua` (DCS-BIOS), `Nav/nav.lua`, `Systems/shrike.lua`

## Controls Inventory

| DCS Point | Panel Label | DCS-BIOS ID | Switch Type | Positions / Range | Device |
|---|---|---|---|---|---|
| 720 | FUEL / EXT | `FUEL_EXT_BTN` | Pushbutton (momentary) | momentary | Avionics (dev 2) |
| 721 | PROFILE / PLAN | `RADAR_PROFILE` | 2-pos toggle | PROFILE / PLAN | Radar Scope (dev 10) |
| 722 | LONG / SHORT | `RADAR_RANGE` | 2-pos toggle | LONG / SHORT | Radar Scope (dev 10) |
| 723 | TEST | `MASTER_TEST` | Pushbutton (momentary) | momentary | Avionics (dev 2) |
| 724 | NAV PAC / TACAN / NAV CMPTR | `BDHI_MODE` | 3-pos toggle | −1 (NAV PAC) / 0 (TACAN) / +1 (NAV CMPTR) | BDHI (dev 23) |
| 725 | WALLEYE TEST / SHRIKE 1 / SHRIKE 2 | `SHRIKE_SEL_KNB` | 5-pos rotary | 0.0 / 0.1 / 0.2 / 0.3 / 0.4 | Armament (dev 6) |
| 726 | VOL | `MISSILE_VOL` | Pot (continuous) | −1.0 to +1.0 | Armament (dev 6) |
| 727 | CONT / NORM | — | 2-pos toggle | — | — (spare GPIO) |

**Notes:**
- Point 725: 5 positions defined in code; `SetCommand` handler is a stub — selector not functionally implemented in mod. All 5 positions wired physically.
- Point 724: "BDHI" is the instrument name, not a switch position. Default centre position = TACAN.
- Point 727: No DCS-BIOS entry; physical switch exists in model. Wired to spare GPIO for future use.
- Point 726: Pot wiper routed via J2 pin 7 to STM32 ADC on MCU board — no ADC chip on this breakout.

## I/O Summary

| Type | Count | Points | GPIO pins | Implementation |
|---|---|---|---|---|
| Momentary pushbutton | 2 | 720, 723 | 2 | MCP23017 |
| 2-pos toggle | 2 | 721, 722 | 2 | MCP23017 |
| 3-pos toggle | 1 | 724 | 2 | MCP23017 (common GND; detect low on A or B) |
| 5-pos rotary | 1 | 725 | 5 | MCP23017 (one GPIO per position, common GND) |
| 2-pos toggle (spare) | 1 | 727 | 1 | MCP23017 |
| **Total MCP23017 GPIO** | | | **12** | One MCP23017 (16 GPIO), 4 spare |
| Continuous pot | 1 | 726 | — | Analog wiper → J2 pin 7 → STM32 ADC |

## Harness Connector (to MCU board)

**J2 — 7-pin Molex MicroFit 3.0**

| Pin | Signal |
|---|---|
| 1 | SDA |
| 2 | SCL |
| 3 | GND |
| 4 | GND |
| 5 | 12 V (LED PWM) |
| 6 | 3.3 V |
| 7 | ANALOG (MISSILE_VOL wiper) |

## Switch-to-PCB Connectors

JST-XH (2.54 mm pitch). Grouping TBD during PCB layout — group by physical proximity on panel.

## LEDs

5050 SMD red, PCB front face, arrays of 5, driven at ~30 mA, powered from 12 V PWM line (J2 pin 5).

## Dimensions

Model viewer corner coordinates (manually recorded; 1 model unit = 1000 mm, × 1.10 scale factor):

| Corner | X | Y | Z |
|---|---|---|---|
| Bottom left | 0.434 | −0.427 | −0.071 |
| Bottom right | 0.427 | −0.425 | 0.096 |
| Top left | 0.462 | −0.369 | −0.071 |

| Measurement | Model units | Real-world (mm) |
|---|---|---|
| Width (BL → BR) | 0.1672 | ~184 mm (to be confirmed in Fusion 360) |
| Height (BL → TL) | 0.0644 | 71 mm |

Switch centre-to-centre spacings TBD from Fusion 360 model.
