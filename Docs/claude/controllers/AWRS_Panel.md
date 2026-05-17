# AWRS Panel

**Location:** Center Console, Armament section  
**Controller:** Center_Armament (breakout via J1)  
**Repo:** `PCB/Center_Console/Center_Armament/AWRS_Panel/`

## Controls Inventory

Controls inventory, switch types, and DCS-BIOS IDs TBD — future plan.

Known inputs from `Center_Armament.md`:

| Input | Type | Implementation |
|---|---|---|
| QTY | TBD | ADS1115 channel (resistor ladder or pot) |
| DROP INTVL | TBD | ADS1115 channel (resistor ladder or pot) |
| MODE SEL | TBD | ADS1115 channel (resistor ladder or pot) |

## I²C Device

| Address | Device | Notes |
|---|---|---|
| 0x48 | ADS1115 | ADDR pin tied to GND; 4-channel 16-bit ADC |

## Harness Connector (to Armament_MCU)

**J1 — 6-pin JST-XH** (standard intra-group harness)

| Pin | Signal |
|---|---|
| 1 | SDA |
| 2 | SCL |
| 3 | GND |
| 4 | GND |
| 5 | 3.3 V (chip power) |
| 6 | spare |

## LEDs

TBD — backlighting requirements not yet determined.

## Dimensions

TBD from Fusion 360 model.
