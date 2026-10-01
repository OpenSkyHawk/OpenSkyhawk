# DrumDisplay — Technical Specification

**Status:** Done (hardware-verified — mux + readouts on real OLEDs, 2026-06-22; bench-fix PR #136). + I2C circuit breaker #164 (compile-gated; bench pending)
**FirmwarePlan ref:** issue #113 (OLED drum readout)
**Depends on:** `PanelGroup.md`, `Helpers/I2cMux.md`, `olikraus/U8g2`

---

## Responsibility

Rolling mechanical-drum OLED readout. One `DrumDisplay` instance drives **one** OLED panel
through a caller-owned `U8G2` object, rendering a row of digit "tapes" that ease toward a target
value with a drum cascade (units rolls continuously; higher places dwell and roll on carry),
optionally followed by a 2-state hemisphere/mode flag tape. Multi-digit DCS-BIOS readouts
(APN-153 SPEED, NAV LAT/LON, MAGVAR, ARC-51 frequency, BDHI range) are reconstructed from their
per-digit DCS-BIOS output addresses via a `DrumReadout` descriptor. Self-registers into
PanelGroup's `OutputBase` list.

Does **not** own the panel — the sketch constructs the concrete `U8G2` type (it knows SH1106 vs
SSD1306, size, rotation), calls `Wire.begin()` + `oled.begin()`, and passes the panel by
reference. Does **not** draw in `onControlPacket()` (the full-buffer I²C send is the expensive op
and is deferred to `update()`). Does **not** interpret what a readout *means* — the descriptor maps
addresses to columns.

The renderer (drawTape / drawFlag / per-place ease / per-cell clip) is ported from the
bench-verified SH1106 prototype (`Tests/DrumDisplay/tests/roll_reference`). The class adds three
things the prototype lacked: **DCS-BIOS address→digit decode**, **snap-then-settle**, and
**descriptor-driven geometry auto-fit** (replacing the prototype's hardcoded pixel constants).

---

## File Layout

```
Firmware/Libraries/DrumDisplay/        — separate library (deps: PanelGroup + U8g2)
├── DrumDisplay.{h,cpp}
└── library.json

Firmware/Libraries/PanelGroup/
└── Helpers/I2cMux/I2cMux.{h,cpp}      — TCA9548A selector, stays in PanelGroup (Wire-only)
```

**DrumDisplay is its own opt-in library, not part of PanelGroup** — required, not cosmetic. Panel
projects build PanelGroup with `lib_ldf_mode = deep+`, which compiles *every* PanelGroup source, so a
`DrumDisplay.cpp` inside PanelGroup forces `<U8g2lib.h>` onto every panel node and breaks the build of
non-display projects (CI-proven on Center_Armament + E2E_PanelGroup). As a standalone library, only
projects that list `file://../../Libraries/DrumDisplay` in their `lib_deps` compile it and pull U8g2;
all other PanelGroup nodes stay U8g2-free. `I2cMux` stays in PanelGroup (needs only `Wire`, so it
compiles everywhere harmlessly).

**Readout descriptors** (`DrumReadout` tables) are **not** a shared header — each is defined in the
sketch that drives that panel, alongside its `PinRef` / U8G2 wiring (see Sketch Usage). A node
defines only the readouts it actually shows.

### Test project

```
Firmware/Tests/DrumDisplay/
├── platformio.ini                  — env_base + U8g2/A4EC deps + -DDRUMDISPLAY_TEST; bluepill_f103c8 bench env
└── tests/
    ├── bringup/                    — OledBringup port: counter + big font (panel smoke test)
    ├── roll_reference/             — OledRoll port: rolling tapes + 2-face flag (renderer regression)
    ├── speed/                      — APN-153 3-digit; decode/splice → 250; bring-up gate
    ├── nav_pos/                    — current longitude: 6 digits + E/W flag; auto-shrink fires
    ├── magvar/                     — magnetic variation: 5 digits + E/W flag
    ├── grouped_decimal/            — altimeter setting: 2-digit source splice + '.' glyph → 29.92
    ├── arc51_manual/               — ARC-51 manual freq: two sources share one address (mask-split)
    ├── bdhi/                       — BDHI DME range: 3 digits + a dedicated 2-state flag source
    ├── font/                       — SMALL/LARGE + a 128x32 panel; runtime setFontSize() re-fit
    ├── mux/                        — two panels on one I2cMux; independent decoded state
    ├── descriptor_guard/           — out-of-bounds descriptors rejected + inert; a valid one renders
    └── decode_bands/               — real captured values: every digit 0..9, band edges, clamping
```

Compile-gated in CI on `genericSTM32F103C8`; logic asserts run via the `check()`→`diagSerial()`
PASS/FAIL idiom (`-DDRUMDISPLAY_TEST` exposes `debugTarget()` / `debugCellCount()` /
`debugRowWidth()` / `debugFlagTarget()` / `debugDescriptorOk()`). The `bluepill_f103c8` env is flashed to a real SH1106
for the on-hardware bench pass.

---

## Public API

```cpp
// Firmware/Libraries/DrumDisplay/DrumDisplay.h  (inside #ifdef ARDUINO_ARCH_STM32)
#include <PanelGroup.h>      // OutputBase
#include <U8g2lib.h>
#include <Helpers/I2cMux/I2cMux.h>

namespace OpenSkyhawk {

enum class DrumFont   : uint8_t { SMALL = 0, LARGE = 1 };          // profont22_mr / profont29_mr
enum class DrumScroll : uint8_t { EASE_ONLY = 0, SNAP_SETTLE = 1 };

struct DrumSource { uint16_t address; uint16_t mask; uint8_t nDigits; uint8_t place; };
struct DrumGlyph  { char ch; uint8_t afterCol; float widthMm; };
struct DrumFlag   { bool enabled; uint16_t address; uint16_t mask;
                    const char* faces; uint8_t atVisualCol; float widthMm; };
struct DrumReadout {
    const DrumSource* sources; uint8_t nSources; uint8_t nDigits;
    float digitWidthMm, digitHeightMm, interDigitGapMm, groupGapMm; uint8_t groupSize;
    const DrumGlyph* glyphs; uint8_t nGlyphs;
    DrumFlag flag; DrumScroll scroll; float snapThreshold;
};

class DrumDisplay : public OutputBase, public I2cHealth, public FaultSource {  // FaultSource: #163
public:
    // Direct-bus on the default trunk (Wire).
    DrumDisplay(U8G2& oled, const DrumReadout& readout,
                DrumFont font = DrumFont::LARGE, float xOffsetMm = 0.0f, float yOffsetMm = 0.0f);
    // Direct-bus on a stated trunk — needed when a node drives both I²C buses at once.
    DrumDisplay(U8G2& oled, const DrumReadout& readout, TwoWire& wire,
                DrumFont font = DrumFont::LARGE, float xOffsetMm = 0.0f, float yOffsetMm = 0.0f);
    // Muxed: one panel behind a TCA9548A branch (re-selects its channel before each I²C op).
    DrumDisplay(U8G2& oled, const DrumReadout& readout, I2cMux& mux, uint8_t channel,
                DrumFont font = DrumFont::LARGE, float xOffsetMm = 0.0f, float yOffsetMm = 0.0f);

    void configure() override;                                  // auto-fit geometry, blank
    void onControlPacket(uint16_t controlId, uint16_t value) override;  // decode + dirty, never draws
    void update() override;                                     // ~60fps gate, ease+snap, render

    void setFontSize(DrumFont font);                            // runtime; re-fits next frame
    void setOffset(float xOffsetMm, float yOffsetMm);          // runtime mm; re-registers next frame

    // FaultSource (NodeStatus.h) — DrumDisplay is one node fault source among many (#163):
    NodeFaultCode faultCode() const override;  // I2C_PERIPHERAL when breaker tripped, else NONE
    const char* faultDetail() const override;  // DiagSerial-only hop string (Mux/Device)
};

}  // namespace OpenSkyhawk
```

All addressing/masking uses the `A_4E_C_*` / `A_4E_C_*_AM` constants from `A4EC_OutputIds.h` —
never raw hex. Each `DrumReadout` is defined in the sketch that drives the panel (panel wiring), not
a shared global.

---

## Sketch Usage

The readout descriptor is defined in the sketch, like the `PinRef` wiring map — a node defines only
the readouts it drives.

```cpp
#include <Wire.h>
#include <PanelGroup.h>
#include <DrumDisplay.h>          // separate lib — add file://../../Libraries/DrumDisplay to lib_deps
#include <A4EC_OutputIds.h>       // A_4E_C_* addresses + _AM masks

using namespace OpenSkyhawk;

// ── readout descriptor (panel wiring, defined here, not a shared global) ──
static const DrumSource SPEED_SRC[] = {
    { A_4E_C_APN153_SPEED_X00, A_4E_C_APN153_SPEED_X00_AM, 1, 2 },
    { A_4E_C_APN153_SPEED_0X0, A_4E_C_APN153_SPEED_0X0_AM, 1, 1 },
    { A_4E_C_APN153_SPEED_00X, A_4E_C_APN153_SPEED_00X_AM, 1, 0 },
};
static const DrumReadout APN153_SPEED = {
    SPEED_SRC, 3, 3, 4.5f, 8.0f, 1.0f, 0.0f, 0, nullptr, 0,
    { false, 0, 0, nullptr, 0, 0.0f }, DrumScroll::SNAP_SETTLE, 3.0f,
};

// Sketch owns the concrete panel type, rotation, and Wire.
U8G2_SH1106_128X64_NONAME_F_HW_I2C oledSpeed(U8G2_R0, U8X8_PIN_NONE);
DrumDisplay speed(oledSpeed, APN153_SPEED, DrumFont::LARGE);

void setup() {
    Wire.begin();                        // I2C1 = PB6/PB7 on the base board (core defaults)
    oledSpeed.setI2CAddress(0x3C << 1); oledSpeed.begin();
    PanelGroup::setup();   // calls configure() on every DrumDisplay (auto-fits + blanks)
}
void loop() { PanelGroup::loop(); }   // dispatches CTRL_BCAST → onControlPacket; calls update()
```

**One transport argument: a bus, or a mux.**

Several panels on one TCA9548A — each re-selects its channel before every buffer send:

```cpp
#include <Helpers/I2cMux/I2cMux.h>

I2cMux navMux(0x70, Wire);                 // the mux carries the bus
DrumDisplay lat(oledLat, LAT_READOUT, navMux, /*channel*/ 0);
DrumDisplay lon(oledLon, LON_READOUT, navMux, /*channel*/ 1);
```

A single panel on the second trunk. The core does not predefine a `Wire1` for these variants, so
the sketch declares one; pass it to the ctor so the probe follows the panel:

```cpp
TwoWire Wire1(PB11, PB10);                 // J_I2C2 — SDA, SCL
U8G2_SSD1306_128X32_UNIVISION_F_2ND_HW_I2C oledSpeed(U8G2_R0, U8X8_PIN_NONE);
DrumDisplay speed(oledSpeed, APN153_SPEED, Wire1);
```

A bare OLED may not share a trunk with same-address panels behind a mux — it is shadowed whenever
a channel is open (`hardware-standards.md`). Put the mux on one trunk and the bare panel on the
other.

---

## Key Data Structures

| Struct | Role |
|---|---|
| `DrumSource` | One DCS-BIOS address → `nDigits` digits at `place`. `mask` (an `_AM` constant) extracts a field; two sources may share one address (ARC-51 10/1 MHz). `steps`/`mul`/`offset` give the band (see onControlPacket); omit them for a plain digit drum. |
| `DrumGlyph` | A fixed, non-rolling glyph (decimal point) in its own cell at `afterCol`. |
| `DrumFlag` | Optional 2-state tape; configurable position (`atVisualCol`) and faces. Default off. |
| `DrumReadout` | The whole readout: sources, geometry (mm), glyphs, flag, scroll mode + threshold. |

Geometry is mm-based and resolution-independent — `configure()` converts mm → px from the panel's
`getDisplayWidth()/Height()`, so one descriptor serves 0.91" / 0.96" / 1.3" panels.

---

## Implementation Notes

### onControlPacket() — decode + splice, never draws

A source value is mapped onto its **band**, then onto what the drum shows:
`shown = idx · mul + offset`, where `idx` is the band the value falls in. The decoded field is
spliced into the combined `_target` at its `place` via keep-high / part / keep-low, then `_dirty`
is set.

**Why a band and not a scale (#137).** DCS-BIOS segments nothing of its own: `Module.valueConvert()`
maps the gauge's declared arg range linearly onto 0..65535 and `MemoryAllocation:setValue()` floors
the result. The meaning is the gauge's, and the A-4E-C gauges do not agree:

| source | export | `steps` / `mul` / `offset` |
|---|---|---|
| digit drum (LAT/LON/MagVar/speed/BDHI/ALT_ADJ decimals) | `digit/10` — `utils.lua jumpwheel()` returns `B/10` | 10 / 1 / 0 (the default) |
| ARC-51 50 kHz | 0.00–0.95 in 0.05 steps, declared `{0, 0.95}` | 20 / 5 / 0 |
| ARC-51 10 MHz (float) | 0.05 steps across `{0,1}` | 20 / 1 / 22 |
| ARC-51 selectors (packed) | `defineTumb` exports the POSITION INDEX | 18 or 20 / per drum |
| ALT_ADJ inHg group | `math.floor(alt_setting)` over `{29,30}` | 2 / 1 / 29 |

The old `round(value/mask · (10^nDigits − 1))` matched none of these: a digit drum's 9 arrives as
58981, which it rendered as **8** — every digit from 5 up read one low. `steps` defaults to
`10^nDigits`, so a plain drum needs no extra fields.

Two mapping rules, picked by the mask, matching what the DCS-BIOS Arduino library does:

- **whole word (`0xFFFF`)** — a `defineFloat`, so the value is the arg normalised onto the range:
  `idx = ⌊value/65535 · steps + BAND_EPS⌋`. `BAND_EPS` undoes the `math.floor` on the export side,
  which otherwise leaves a clean band edge one LSB short. A roll fraction `(B+dd)/10` stays inside
  its band, so the cascade still animates from the right digit.
- **packed field** — a `defineTumb`/`defineMultipositionSwitch`, which allocates a small integer,
  so the field already *is* the index: right-justify it (`(value & mask) >> shift`, exactly
  `IntegerBuffer::getData()` upstream) and use it. Scaling it would be wrong.

`idx` is clamped to `steps − 1`, matching `defineTumb`'s own clamp to `last_n`. The loop scans **all** sources (not just the first match) so
two sources sharing one address both splice. The flag branch does **not** early-return, so an
address that is *both* a digit and the hemisphere flag (NAV dual-role) updates both. Drawing is
deferred to `update()` (full-buffer I²C is the expensive op — the `LED.cpp` store-only idiom).

### configure() — descriptor validation (#137)

`_pos[6]` and `_cellX[MAX_CELLS]` are fixed arrays and a `DrumReadout` is hand-authored, so
`configure()` validates the descriptor before laying anything out:

- `nDigits` within **1..6** — the depth of `_pos[]`;
- total visual cells (digits + glyphs + the flag) ≤ `MAX_CELLS` (8);
- every source fits the readout it splices into: `place + nDigits <= readout.nDigits`;
- every source's band fits its columns: `(steps − 1) · mul + offset <= 10^nDigits − 1`, with a
  non-negative `offset` — a mis-copied band is otherwise a quietly wrong readout.

A failing descriptor is **rejected, not clamped**: the offending field is logged on DiagSerial and
the readout is disabled — no cells are laid out, and `onControlPacket()` / `update()` return
immediately, so neither the decode state nor the ease loop can touch the arrays. The ease loop is
why this matters at render time and not only at layout: it walks `_r->nDigits` entries of `_pos[]`,
so an `nDigits` of 7 writes past the array on every frame. `debugDescriptorOk()` reads the verdict.

### fitGeometry() — descriptor-driven auto-fit (replaces the prototype's constants)

Converts the descriptor's mm fields to px at a nominal 4.35 px/mm, lays out the visual cells
(digits leftmost = highest place, glyphs at `afterCol`, flag at `atVisualCol`, group gaps), and if
the row is wider than the panel **uniformly shrinks** pitch to fit the pixel width. The roll-window
height is clamped to short (128×32) panels. The block is centred, then offset by the per-mounting
`xOffsetMm`/`yOffsetMm` (mm → px at the nominal scale; ~0.25 mm ≈ 1 px is the smallest useful step — the
0.91" is ~0.175 mm/px, the 1.3" ~0.23 mm/px). Re-run on the next frame whenever `setFontSize()` /
`setOffset()` mark geometry dirty — `update()`'s idle skip honors that geom-dirty flag, so a runtime
re-register applies **even on a settled readout** (was a no-op before — bench-found, fixed).

### update() — frame gate, idle skip, ease + snap, render

~60 fps gate; **early-out when settled and not dirty** (no I²C write when nothing moves). Each
place eases toward `target / 10^place` (the proven cascade). **SNAP_SETTLE**: when a place is more
than `snapThreshold` digits from its target, the tape teleports to ~1.5 digits shy (preserving roll
direction) then eases the last detent — so a 130→250 jump shows one detent, not 120 spins. Small
deltas fall straight through to the ease. Rendering selects the mux channel, clips each cell, draws
its tape/glyph/flag, and issues one `sendBuffer()`.

### Mux ordering

A muxed instance calls `mux.select(channel)` before the blank in `configure()` and before the
`sendBuffer()` in `update()`, so interleaved panels sharing identical 0x3C addresses never receive
the wrong buffer. `I2cMux::select()` caches the last channel and skips redundant writes.

### I2C circuit breaker (#164)

`DrumDisplay` mixes in `I2cHealth` and gates every render (the `configure()` blank + the `update()`
send) behind `i2cReachable()`. `i2cProbe()`:

- **muxed:** a **forced** `select(channel, true)` (uncached write — confirms the mux *and* re-routes,
  so a mux reset / power-glitch recovers) then `deviceAcks(oledAddr)` for the OLED on the branch —
  classifying the failure `Fault::Mux` vs `Fault::Device` (feeds #163).
- **direct-bus:** probes the OLED address on the trunk the ctor was given — `Wire` unless a
  `TwoWire&` was passed. The U8G2 object already carries its own bus, so the handle exists to keep
  the probe on the same one; mismatch them and the breaker faults a working panel.

A dead/absent panel → the render is skipped and the breaker backs off ~2 s between re-probes, so a
missing OLED can no longer stall `PanelGroup::loop()` and flap the node. The decode path
(`onControlPacket`) does no I2C, so the value stays current and the first reachable frame catches up —
no stale freeze. The OLED address is read from `_oled->getU8x8()->i2c_address`. Pair with
`-DI2C_TIMEOUT_TICK=10` on the sketch so each transaction is bounded (the breaker bounds *frequency*).

---

## Dependencies

| Dependency | Source | Notes |
|---|---|---|
| PanelGroup | `Firmware/Libraries/PanelGroup` | OutputBase, dispatch; CTRL_BCAST handled by PanelGroup |
| U8g2 | `olikraus/U8g2@^2.35` | Full-buffer driver; caller constructs the concrete type |
| I2cMux | `PanelGroup/Helpers/I2cMux` | Optional TCA9548A selector for multi-panel nodes |
| A4EC | `Firmware/Libraries/A4EC` | `A_4E_C_*` addresses + `_AM` masks (`A4EC_OutputIds.h`); descriptors are per-sketch |

### A-4E-C readout encodings — settled (#137)

Read out of the mod + DCS-BIOS sources and confirmed against two recorded sim captures, where each
float drum was paired with its selector's exported position index:

- **The rightmost LAT/LON/MagVar output is the hemisphere, never a digit.** `nav.lua` sets it from
  `N_S` / `E_W`, so longitude is 5 digits + flag, latitude 4 + flag, MagVar 4 + flag. In a captured
  session `LON_0000X0` took 2044 values while `LON_00000X` never left `32767`.
- **Flag scales differ within one aircraft.** ASN-41 position flags are **half scale** (0.0 / 0.5 →
  `steps = 2`); MagVar and the BDHI DME flag are full scale (0 / 1 → `steps = 1`). A half-scale flag
  read as full scale never reaches its second face.
- **ARC-51 outputs are selector positions**, not grouped 00–99 values: 10 MHz → 22–39, 1 MHz → a
  digit, 50 kHz → 00,05…95. Display is XXX.XX, so the dot sits after the third digit.
- **Decimal positions** confirmed: ALT_ADJ `29.92` (dot after 2), frequency `299.90` (dot after 3).

Verified by `decode_bands` against the values the captures carry, not synthesised ones.
