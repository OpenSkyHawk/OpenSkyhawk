# AN/ASN-41 Navigation Computer

**Location:** Right Console, Navigation section
**Controller:** Right_Navigation (NODE_ID 2) — **host panel** (carries the STM32F103 PanelGroup MCU for the whole node)
**Repo:** `PCB/Right_Console/Right_Navigation/ASN-41/` (scaffolded at B2)
**Tracking:** panel issue [#169](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/169) · controller [#168](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/168)
**Stage:** B1 Research complete → B2 Schematic next

Inertial/Doppler navigation computer for continuous en-route and attack navigation. The pilot
pre-sets present position, two destination waypoints (D1/D2), magnetic variation and wind data
with seven multi-turn push-to-set knobs; two spring-loaded slew knobs trim lat/lon in flight.
Seven OLED readouts show present/destination position, mag var, and wind. As the controller
**host**, this board carries the NODE-2 MCU and both I²C buses: I²C1 = its own I/O, I²C2 = the
harness out to the APN-153 and ARC-51 sub-panels.

## Controls Inventory

**24 controls** — 17 inputs · 7 displays · 0 LEDs

### Inputs

| Control | Identifier | DCS-BIOS (addr/mask) | FW class | Interaction |
|---|---|---|---|---|
| Function Selector | `NAV_SEL` | 0x8522 / 0x0038 | `AnalogMultiPos` | 5-pos OFF/STBY/D1/D2/TEST; resistor ladder → host ADC PA0; selects which dest (D1/D2) the Dest readout shows |
| Present Lat knob / push | `PPOS_LAT_KNB` / `PPOS_LAT_BTN` | 0x8524 · 0x8522/0x0040 | `RotaryEncoder` REL + `Switch2Pos` | push-to-set, multi-turn |
| Present Lon knob / push | `PPOS_LON_KNB` / `PPOS_LON_BTN` | 0x8526 · 0x8522/0x0080 | `RotaryEncoder` REL + `Switch2Pos` | push-to-set |
| Dest Lat knob / push | `DEST_LAT_KNB` / `DEST_LAT_BTN` | 0x8528 · 0x8522/0x0100 | `RotaryEncoder` REL + `Switch2Pos` | push-to-set |
| Dest Lon knob / push | `DEST_LON_KNB` / `DEST_LON_BTN` | 0x852A · 0x8522/0x0200 | `RotaryEncoder` REL + `Switch2Pos` | push-to-set |
| Mag Var knob / push | `ASN41_MAGVAR_KNB` / `_BTN` | 0x852C · 0x8522/0x0400 | `RotaryEncoder` REL + `Switch2Pos` | push-to-set |
| Wind Speed knob / push | `ASN41_WINDSPEED_KNB` / `_BTN` | 0x852E · 0x8522/0x0800 | `RotaryEncoder` REL + `Switch2Pos` | push-to-set |
| Wind Dir knob / push | `ASN41_WINDDIR_KNB` / `_BTN` | 0x8530 · 0x8522/0x1000 | `RotaryEncoder` REL + `Switch2Pos` | push-to-set |
| Dest Lat Slew | `ASN41_LAT_SLEW` | 0x8544 / 0x0060 | `Switch3Pos` | spring-centred momentary L/R |
| Dest Lon Slew | `ASN41_LON_SLEW` | 0x8544 / 0x0180 | `Switch3Pos` | spring-centred momentary L/R; concentric with the push-set encoder |

`0x8522` is a packed word — NAV_SEL + the 7 pushes on distinct bitmasks. `NAV_DEAD` (0x8554) is a different panel — excluded. 25 digital input lines + 1 ADC.

**Encoder REL step (bench, PR #165):** the 4 position knobs (PPOS/DEST lat-lon) use `step=1600` = 1 digit/detent — `step=3200` (the JSON suggested_step) over-steps. MagVar/WindSpeed/WindDir default 3200 pending per-knob feel (likely also 1600). Set per-ctor in the B6 sketch; class `DEFAULT_STEP` stays 3200.

### Outputs — 7 OLED readouts

| Readout | Identifiers | Digits | Flag | FW class |
|---|---|---|---|---|
| Present Latitude | `NAV_CURPOS_LAT_*` (0x8484–8C) | 5 | N/S | `DrumDisplay` |
| Present Longitude | `NAV_CURPOS_LON_*` (0x848E–98) | 6 | E/W | `DrumDisplay` |
| Destination Latitude | `NAV_DEST_LAT_*` (0x849A–A2) | 5 | N/S | `DrumDisplay` |
| Destination Longitude | `NAV_DEST_LON_*` (0x84A4–AE) | 6 | E/W | `DrumDisplay` |
| Mag Var | `ASN41_MAGVAR_*` (0x84BC–C4) | 5 | W/E | `DrumDisplay` |
| Wind Speed | `ASN41_WINDSPEED_*` (0x84B0–B4) | 3 | — | `DrumDisplay` |
| Wind Direction | `ASN41_WINDDIR_*` (0x84B6–BA) | 3 | — | `DrumDisplay` |

All OLED (per #113) — no motorized drums. Hemisphere flags via the `DrumDisplay` flag cell.

## I²C Devices (I²C1, host)

| Address | Device | LCSC | Notes |
|---|---|---|---|
| 0x20, 0x21 | MCP23017-E/SS ×2 | C558584 | 25 input lines (14 usable/chip; GPA7/GPB7 output-only). INT_A/INT_B → host PB12/PB13 |
| 0x70 | TCA9548A | C130026 | subs the 7 OLED readouts (all 0x3C behind mux) |
| 0x3C (behind mux) | OLED ×7 | AliExpress | the 7 readouts; sizes per window at B3 |

**Host STM32 pins:** PA0 = NAV_SEL ladder · PA2 = ARC-51 VOL passthrough · ~2 breakout GPIO reserved as ARC-51 native input lines (1 encoder) — assigned at B2. ~11 of 13 breakout GPIO free.

**No steppers, no DRV8833** on this board (the only Right_Navigation stepper is APN-153's DRIFT needle).

**Input backend alternative — 74HC165 ([#197](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/197)):** input-heavy host with 7 REL encoders → strong candidate for the SPI '165 backend (off I²C, no OLED-flush contention). MCP23017 = proven fallback.

## Open items / gates

- **Encoder-fidelity bench gate (before B5):** 7 REL encoders on MCP23017/I²C1 — unproven in our stack (proven runs were direct GPIO); REL accumulates, so a missed count drifts. Bench with the 7 OLEDs animating; 74HC165 (#197) resolves it.
- **B3 confirms:** panel dimensions · 7 OLED window sizes · knob center-to-center · concentric slew+encoder assembly · hemisphere-flag glyph/position.
- **D1/D2** — single Dest readout shows the NAV_SEL-selected waypoint (confirm in sim).
- **Host 2-native-GPIO** assignment for ARC-51 finalized at B2.
