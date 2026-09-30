# AN/APN-153 Doppler Navigation

**Location:** Right Console, Navigation section
**Controller:** Right_Navigation (NODE_ID 2) — I²C2 sub-panel; host MCU on AN/ASN-41
**Repo:** `PCB/Right_Console/Right_Navigation/APN-153_Doppler/` (scaffolded at B2)
**Tracking:** panel issue [#170](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/170) · controller [#168](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/168)
**Stage:** B2 Schematic complete — ERC clean ([#181](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/181)) → B3 CAD next

The Doppler navigation radar set: pilot selects radar mode (OFF/STBY/LAND/SEA/TEST),
reads drift angle on a needle gauge and ground speed on a 3-digit readout, with a MEMORY
lamp indicating the set is holding last-known velocity. Feeds ground-speed + drift to the
AN/ASN-41 nav computer. No MCU of its own — routes to the ASN-41 host board over a standard
8-pin I²C harness on I²C2.

## Controls Inventory

**5 controls** — 2 inputs · 2 gauges · 1 LED _(+ 2 cosmetic DA/GS knobs, no I/O)_

### Inputs

| Control | Identifier | DCS-BIOS | FW class | Interaction |
|---|---|---|---|---|
| Nav Mode Selector | `DOPPLER_SEL` | 0x8522 | `SwitchMultiPos` | 5-pos rotary OFF/STBY/LAND/SEA/TEST; notch-per-click, TEST detented (holds). ALPHA SR26-series, 1-pole 12-pos, 30°, non-shorting — **panel-mounted** (3/8-32 bushing), **wired to PCB via J2** (not PCB-soldered → clearance); prefer **SR2611F** solder-lug variant. 5 positions used, limited via the switch stop-washer |
| Memory Light Test | `DOPPLER_MEM_TEST` | 0x8512 | `Switch2Pos` | momentary press; illuminated pushbutton (same part as MEMORYLIGHT) |
| Drift Angle knob (DA) | model arg 243 | — | — cosmetic | non-interactive — CAD mock-up only, no I/O |
| Ground Speed knob (GS) | model arg 245 | — | — cosmetic | non-interactive — CAD mock-up only, no I/O |

### Outputs

| Display group | Identifiers | FW class | Drive |
|---|---|---|---|
| Drift needle | `APN153_DRIFT_GAUGE` (0x847C) | `NeedleGauge` | X27.589 stepper via DRV8833; ≈236° sweep (~708 steps full-scale). **MCP-driven → step-rate gated by [#190](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/190)** |
| Ground Speed | `APN153_SPEED_X00/_0X0/_00X` (0x847E/80/82) | `DrumDisplay` | OLED (I²C, SSD1306 0.91″ 128×32, PCB-mounted) — no motor; U8G2 ctor = SSD1306 128×32 |
| Memory Light (yellow) | `APN153_MEMORYLIGHT` (0x8470) | `LED` | integrated in MEMORY pushbutton; driven solely by DCS-BIOS output |

## I²C Devices (on I²C2)

| Address | Device | LCSC | Notes |
|---|---|---|---|
| 0x20 | MCP23017-E/SS | C506653 | Port A: GPA0–5 = 6 in (DOPPLER_SEL ×5 + MEM), GPA6 = DRV8833 ~SLEEP (sim-gated, 10 kΩ pull-down), GPA7 = MEMORYLIGHT; Port B: GPB0–3 = DRIFT stepper coils. 12/16 used. INT_A → host PB8 |
| 0x3C | OLED SSD1306 0.91″ 128×32 (module) | TBD | GND SPEED via `DrumDisplay` (controller OLED addressing / mux on #168). VCC 3.3–5 V (onboard reg). **PCB-mounted** (tentative — confirm at CAD/B3); own library part (symbol + footprint + STEP). 0.91″ bench-verified on real APN-153 faceplate |

## Discrete Drivers

| Device | Qty | LCSC | Purpose |
|---|---|---|---|
| DRV8833PWPR | 1 | C50506 | DRIFT needle stepper (1 dual-H-bridge); `~SLEEP` ← MCP GPA6, sim-gated |
| Stepper X27.589 | 1 | TBD | DRIFT air-core needle |

DRIFT stepper coils + its MCP stay **local** to this board — they do not cross the harness.

## Harness (to ASN-41 host)

Standard 8-pin JST-XH on **I²C2**:

| Pin | Signal |
|---|---|
| 1 | SDA |
| 2 | SCL |
| 3 | GND |
| 4 | GND |
| 5 | +3V3 |
| 6 | INT_A → host PB8 |
| 7 | INT_B (unused) |
| 8 | +5V (DRV8833 VM — stepper supply) |

Pure-I²C harness: `DOPPLER_SEL` is digital `SwitchMultiPos` → no host ADC, no host expander growth.

## Dimensions & Mounting

- **Panel:** 146.05 × 57.15 mm — standard Dzus single-width (5¾″) × 6 Dzus units; 1⁄16″ aluminum.
- **Mounting:** 4× Dzus quarter-turn studs in a 136.5 × 28.575 mm pattern (MIL-F-25173A, `PR 3½`
  receptacle strip + .375″-head / .257″-body stud), on the 3⁄8″ rail grid. See
  `docs/hardware/mechanical-standards.md`.

## Backlight

LED array count TBD from CAD lit-area map (B4). Backlight MOSFET zones + PWM nets defined at B2;
LED power via the controller backlight rail. No MOSFET on this sub-panel — zone switching on the host board.

## Prerequisites — all met

`DrumDisplay` · `SwitchMultiPos` · `Switch2Pos` · `NeedleGauge` · `LED` all implemented and (gauges)
bench-verified. No firmware/symbol blockers → B6 firmware parallelizable. Only open gate: the
[#190](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/190) MCP23017 NeedleGauge step-rate bench validation, which gates B5 PCB layout.
