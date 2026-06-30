# AN/ARC-51A UHF Radio Control Panel

**Location:** Right Console, Navigation section
**Controller:** Right_Navigation (NODE_ID 2) — I²C2 sub-panel; host MCU on AN/ASN-41
**Repo:** `PCB/Right_Console/Right_Navigation/ARC-51/` (scaffolded at B2)
**Tracking:** panel issue [#171](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/171) · controller [#168](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/168)
**Stage:** B1 Research complete → B2 Schematic next

The AN/ARC-51A UHF command-radio control head. Frequency is set manually with three
detented digit knobs (10 MHz / 1 MHz / 50 kHz) or by preset channel (1–20). A U/H/F lever
picks PRESET / MAN / GD-XMIT and the right rotary is the function selector
(OFF / T-R / T-R+G / ADF), plus volume and a squelch-disable toggle. The frequency window
("220.00 MC") and the preset-channel window are driven readouts. No MCU of its own — routes
to the ASN-41 host board over a standard 8-pin I²C harness on I²C2 (reached via the APN-153
daisy-chain pass-through).

## Controls Inventory

**11 controls** — 8 inputs · 3 displays · 0 LEDs

### Inputs

| Control | Identifier | DCS-BIOS (addr/mask) | FW class | Interaction |
|---|---|---|---|---|
| Freq 10 MHz | `ARC51_FREQ_10MHZ` | 0x853A / 0x03E0 | `RotaryEncoder` DIR | detented knob, no indicator, free-spin (18-pos); emits INC/DEC, sim clamps |
| Freq 1 MHz | `ARC51_FREQ_1MHZ` | 0x853A / 0x3C00 | `RotaryEncoder` DIR | 10-pos; concentric? confirm at B3 |
| Freq 50 kHz | `ARC51_FREQ_50KHZ` | 0x853E / 0x001F | `RotaryEncoder` DIR | 20-pos, 50 kHz steps |
| Preset Channel | `ARC51_FREQ_PRE` | 0x853A / 0x001F | `RotaryEncoder` DIR | separate knob, 1–20 |
| Function Selector | `ARC51_MODE` | 0x853A / 0xC000 | `SwitchMultiPos` | 4-pos OFF/T-R/T-R+G/ADF, ~30°/pos, one-hot, has pointer |
| Frequency Mode | `ARC51_XMIT_MODE` | 0x8532 / 0x3000 | `SwitchMultiPos` | 3-pos PRESET/MAN/GD-XMIT lever, ~30°/pos, one-hot |
| Volume | `ARC51_VOL` | 0x853C / 0xFFFF | `AnalogInput` | continuous pot → host ADC; no built-in push |
| Squelch Disable | `ARC51_SQUELCH` | 0x8532 / 0x4000 | `Switch2Pos` | SQ DISABLE latching toggle |

`0x853A` is a packed 16-bit word — each control on a distinct bitmask, not an address collision.

### Outputs

| Display group | Identifiers | Digits | FW class | Drive |
|---|---|---|---|---|
| Frequency — MHz | `ARC51_FREQ_XX000` + `_00X00` (0x84CE/0x84D0) | 3 (220–399) | `DrumDisplay` | left 1.3″ OLED (SSD1306 128×64) |
| Frequency — kHz | `ARC51_FREQ_000XX` (0x84D2) | 2 (.00–.95) | `DrumDisplay` | right 1.3″ OLED; faceplate prints the "." between |
| Preset Channel | `ARC51_FREQ_PRESET` (0x84D4) | 1–2 | `DrumDisplay` | 0.91″ OLED; leading-zero suppressed ([#196](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/196)) |

Freq format is fixed XXX.XX (225.00–399.95) → the "." is static, printed on the faceplate.
`ARC51_FREQ_PRESET` is a full-word source — bench-verify the decode (raw 1–20 vs 0..65535-scaled).

## I²C Devices (on I²C2)

| Address | Device | LCSC | Notes |
|---|---|---|---|
| 0x21 (+0x22) | MCP23017-E/SS | C558584 | 16 input lines. **14 inputs/chip** (GPA7/GPB7 output-only) → 2 chips, **or 1 chip + 2 native host GPIO** (route one whole encoder native; depends on ASN-41 host spare GPIO). INT → host PB8/PB9 |
| 0x70 | TCA9548A | C130026 | subs the 3 OLEDs (all 0x3C behind the mux) |
| 0x3C (behind mux) | OLED ×3 | AliExpress | 2× 1.3″ (freq) + 1× 0.91″ (channel) |

VOL pot wiper → host STM32 ADC via the harness spare-analog line. No ADS1115, no stepper, no DRV8833.

**Input backend alternative — 74HC165 ([#197](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/197)):** as an all-input panel, ARC-51 is the prime candidate for the SPI 74HC165 backend — takes inputs off I²C (no contention with OLED flushes), removes the 14/chip + native-pin juggling. MCP23017 is the proven fallback until the '165 PinRef backend lands.

## Open items / gates

- **Encoder-fidelity bench gate (before B5):** encoders on MCP23017 are unproven in our stack (all proven runs were direct GPIO); MCP-I²C sampling can be starved by OLED `sendBuffer` flushes on the same bus. Spin all 4 during a live tune and confirm no missed counts. DIR mode is forgiving; 74HC165 (#197) resolves it if MCP fails.
- **#196** — `DrumDisplay` leading-zero suppression, blocks the channel window.
- **B3 confirms:** freq knobs separate-not-concentric (photo says separate; Notes say concentric) · selectors ~30°/pos · freq window ≈57 mm (phone estimate ±15%) · dimensions + knob center-to-center.
- **2nd MCP vs 2 native pins** — settle once ASN-41 host spare-GPIO count is known (#169 B1).
- **Harness** reaches ARC-51 via the APN-153 daisy-chain pass-through ([#198](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/198)).
