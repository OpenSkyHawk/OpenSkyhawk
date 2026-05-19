# AWRS Panel

**Location:** Center Console, Armament section  
**Controller:** Center_Armament (breakout via J1)  
**Repo:** `PCB/Center_Console/Center_Armament/AWRS_Panel/`

## Controls Inventory

| Panel Label | DCS-BIOS ID | Switch Type | Positions / Range | ADS1115 Channel |
|---|---|---|---|---|
| QTY | TBD | TBD (rotary or pot) | TBD | A0 |
| DROP INTVL | TBD | TBD (rotary or pot) | TBD | A1 |
| MODE SEL | TBD | TBD (rotary or pot) | TBD | A2 |

Switch types and DCS-BIOS IDs TBD — pending controls research task (extract from DCS sim). ADS1115 channel assignments (A0/A1/A2) are confirmed design decisions from schematic.

## I²C Device

| Address | Device | Notes |
|---|---|---|
| 0x48 | ADS1115 | ADDR pin tied to GND; 4-channel 16-bit ADC. A3 = NC (spare). |

## Harness Connector (to Armament_MCU)

**J2 — 6-pin JST-XH** (standard intra-group harness → Armament_MCU J1)

| Pin | Signal |
|---|---|
| 1 | SDA |
| 2 | SCL |
| 3 | GND |
| 4 | GND |
| 5 | +3.3V |
| 6 | spare/NC |

No I²C pull-ups on this board — one set only, on Armament_MCU MCU_CAN sheet.

## LEDs

5050 SMD red, PCB front face, strings of 5 in series. LED power via `J4` (2-pin Mini-Fit Jr — `+12V_BACKLIGHT` / `BACKLIGHT_SW_RETURN`). No MOSFET on this board — zone switching lives on Armament_MCU. Array count TBD from Fusion 360 panel model.

## Dimensions

TBD from Fusion 360 model.
