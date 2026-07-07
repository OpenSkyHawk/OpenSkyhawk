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
    └── mux/                        — two panels on one I2cMux; independent decoded state
```

Compile-gated in CI on `genericSTM32F103C8`; logic asserts run via the `check()`→`diagSerial()`
PASS/FAIL idiom (`-DDRUMDISPLAY_TEST` exposes `debugTarget()` / `debugCellCount()` /
`debugRowWidth()` / `debugFlagTarget()`). The `bluepill_f103c8` env is flashed to a real SH1106
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
    // Direct-bus: one panel on the MCU's I²C bus.
    DrumDisplay(U8G2& oled, const DrumReadout& readout,
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
    Wire.setSCL(PB8); Wire.setSDA(PB9); Wire.begin();   // bus pins owned by the sketch
    oledSpeed.setI2CAddress(0x3C << 1); oledSpeed.begin();
    PanelGroup::setup();   // calls configure() on every DrumDisplay (auto-fits + blanks)
}
void loop() { PanelGroup::loop(); }   // dispatches CTRL_BCAST → onControlPacket; calls update()
```

For several panels on one TCA9548A: `#include <Helpers/I2cMux/I2cMux.h>`, construct one
`I2cMux navMux(0x70, Wire);`, and pass it + a channel to each ctor —
`DrumDisplay lat(oledLat, LAT_READOUT, navMux, /*channel*/ 0);` — each re-selects its channel before
every buffer send.

---

## Key Data Structures

| Struct | Role |
|---|---|
| `DrumSource` | One DCS-BIOS address → `nDigits` digits at `place`. `mask` (an `_AM` constant) extracts a field; two sources may share one address (ARC-51 10/1 MHz). |
| `DrumGlyph` | A fixed, non-rolling glyph (decimal point) in its own cell at `afterCol`. |
| `DrumFlag` | Optional 2-state tape; configurable position (`atVisualCol`) and faces. Default off. |
| `DrumReadout` | The whole readout: sources, geometry (mm), glyphs, flag, scroll mode + threshold. |

Geometry is mm-based and resolution-independent — `configure()` converts mm → px from the panel's
`getDisplayWidth()/Height()`, so one descriptor serves 0.91" / 0.96" / 1.3" panels.

---

## Implementation Notes

### onControlPacket() — decode + splice, never draws

A masked source value scales to its digits: `digits = round((value & mask) / mask · (10^nDigits − 1))`.
The decoded field is spliced into the combined `_target` at its `place` via keep-high / part /
keep-low, then `_dirty` is set. The loop scans **all** sources (not just the first match) so
two sources sharing one address both splice. The flag branch does **not** early-return, so an
address that is *both* a digit and the hemisphere flag (NAV dual-role) updates both. Drawing is
deferred to `update()` (full-buffer I²C is the expensive op — the `LED.cpp` store-only idiom).

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
- **direct-bus:** probes the OLED address on `Wire`. (A direct OLED on `Wire1` isn't covered yet — put
  it on the mux, or a follow-up adds a bus handle.)

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

### Bench-confirm TODOs (flagged on the sketch descriptors)

- Hemisphere dual-role on the rightmost LAT/LON/MAGVAR source (digit vs N/S·E/W flag vs both).
- ARC-51 manual selector split (which of `_10MHZ`/`_1MHZ`/`_50KHZ` carries what; linear vs packed).
- Decimal positions for the altimeter setting + displayed frequency.
