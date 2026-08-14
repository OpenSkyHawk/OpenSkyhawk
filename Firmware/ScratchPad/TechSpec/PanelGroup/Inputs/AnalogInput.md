# AnalogInput — Technical Specification

**Status:** Done (hardware-verified — **8/8 envs PASS 2026-08-14** on the assembled PanelGroup Rev 1,
re-run in full after the `pollMs` constructor change (#261). `test_poll_rate` measured
`fast(pollMs=2)=500` / `slow(pollMs=8)=125` reads in a 1000 ms window — nominal, ratio exactly 4.00)
**FirmwarePlan ref:** `FirmwarePlan/05-panelgroup-api.md#analoginput-new`
**Depends on:** `PinRef.md`, `PanelGroup.md`

---

## Responsibility

Continuous analog input — a single analog `PinRef` (potentiometer / axis) normalised to a 16-bit
value 0..65535 and emitted over CAN (`MULTIPOS` transport). A direct `InputBase` subclass — a
**linear** control, *not* a selector: it shares the MULTIPOS wire transport with the selector family
only because DCS-BIOS `set_state` has no separate "continuous" dispatch; the value is the control
position, not an index.

Handles:
- **normalise** — read the ADC (already 16-bit: STM32 ×16 or ADS1115 ×2), clamp to `[minRaw,
  maxRaw]`, map to 0..65535 (reverse-aware);
- **EWMA smoothing** — an integer exponential low-pass (α = 1/2^`ewmaShift`) averages ADC noise,
  which the ×16 STM32 scaling amplifies; a shift, not a divide, so no software-float on the F103;
- **hysteresis + near-rail** — emit only when the smoothed value moves more than `hysteresis` counts
  from the last sent value, or reaches a rail (0 / 65535) moving toward it (so endpoints are always
  reached and a settled pot is silent);
- a **per-instance ADC read throttle** — `pollMs`, default `DEFAULT_POLL_MS` (8 ms), a constructor
  parameter rather than a class constant, so two AnalogInputs on one node may read at different
  rates (`forceReport()` bypasses it for the boot/SYNC baseline).

Does **not** interpret `controlId`, drive a position index, or enable internal pull-ups.

---

## File Layout

```
Firmware/Libraries/PanelGroup/Inputs/AnalogInput/
├── AnalogInput.h
└── AnalogInput.cpp
```

### Test project — `Firmware/Tests/AnalogInput/`

Self-contained — **no analog hardware**. `debugSetRaw(raw)` injects the ADC reading and
`debugStep()` runs one read + EWMA step bypassing the `pollMs` throttle; assertions are on
`value()` (last emitted) / `smoothed()` (current EWMA) / `emitCount()` (CAN EVTs) / `readCount()`
(ADC reads taken — a separate count from emissions, since hysteresis gates emission and a settled
input reads at full rate while emitting nothing) (`#ifdef ANALOGINPUT_TEST`).
CAN runs in **normal mode** so the node ACKs the PanelBridge. No jumpers or pot required.

| Scenario env | Verifies |
|---|---|
| `test_scale_clamp` | `[minRaw,maxRaw]` → 0..65535; out-of-range clamps to the rails; midpoint |
| `test_reverse` | `reverse = true` inverts the mapping (minRaw → 65535) |
| `test_hysteresis` | a sub-`hysteresis` change emits nothing; a larger one emits and tracks |
| `test_ewma` | one EWMA step lands ≈ 1/8 toward the target; many steps converge |
| `test_near_rail` | a **sub-hysteresis** move into a rail band still emits (clause isolated via `emitCount`); a full sweep lands on each rail |
| `test_force_report` | boot read emits the current value once; repeats on a second call |
| `test_shift_bounds` | `ewmaShift` capped at 15 — a full-scale seed does not overflow the int32 acc |
| `test_poll_rate` | two instances with different `pollMs` each hold their own read rate over one wall-clock window (~4:1) — per instance, not per node. The ratio is what fails if `poll()` still compares a class constant |

---

## Public API

```cpp
class AnalogInput : public InputBase {
public:
    static constexpr uint16_t DEFAULT_HYSTERESIS = 128;  // counts on the 16-bit output
    static constexpr uint8_t  DEFAULT_EWMA_SHIFT = 3;    // α = 1/8
    static constexpr uint8_t  MAX_EWMA_SHIFT     = 15;   // scaled << shift must fit int32
    static constexpr uint16_t DEFAULT_POLL_MS    = 8;    // min ms between ADC reads

    /**
     * @param controlId   DCSIN_* or CTRL_* constant. Determines PanelBridge routing.
     * @param pin         analog PinRef (STM32 ADC GPIO or ADS1115 channel).
     * @param reverse     false (default): minRaw → 0, maxRaw → 65535. true: inverted.
     * @param minRaw      raw value mapping to 0 (default 0); below is clamped.
     * @param maxRaw      raw value mapping to 65535 (default 65535); above is clamped.
     * @param hysteresis  output counts of movement before a new value is emitted (default 128).
     * @param ewmaShift   EWMA strength α = 1/2^ewmaShift (default 3 → 1/8).
     * @param pollMs      min interval between ADC reads, ms (default 8). Per instance, not per
     *                    node. Not clamped — 0 = read every loop iteration.
     */
    AnalogInput(uint16_t controlId, PinRef pin, bool reverse = false,
                uint16_t minRaw = 0, uint16_t maxRaw = 65535,
                uint16_t hysteresis = DEFAULT_HYSTERESIS, uint8_t ewmaShift = DEFAULT_EWMA_SHIFT,
                uint16_t pollMs = DEFAULT_POLL_MS);

    void poll() override;          // throttled read + EWMA; emit on hysteresis / rail
    void forceReport() override;   // sample fresh, seed EWMA, emit baseline
    void configure() override;

private:
    void     sample();             // one read + EWMA step + conditional emit
    uint16_t readScaled();         // read ADC, clamp, map → 0..65535
    bool     shouldEmit(uint16_t v) const;
    void     emit(uint16_t v, bool init = false);
    // _pin, _reverse, _minRaw, _maxRaw, _hysteresis, _ewmaShift, _pollMs, _acc, _smoothed, ...
};
```

---

## Sketch Usage

```cpp
#include <PanelGroup.h>
#include <Inputs/AnalogInput/AnalogInput.h>
#include <A4EC_CmdIds.h>

// AN/ARC-51A volume pot on an STM32 ADC pin (harness spare → host ADC). Measure the wiper's
// end-to-end ADC range on the bench and pass it as [minRaw, maxRaw] for full-travel calibration.
OpenSkyhawk::AnalogInput volume(DCSIN_ARC51_VOL, PinRef(PA2), /*reverse=*/false, 300, 65200);

// A HID flight axis wants the opposite trade — finer steps, a faster read, a shorter time
// constant. Requires the 1k + 100nF RC and setDebug(false); see "Read throttle, and the τ
// coupling" below for why each of those binds.
OpenSkyhawk::AnalogInput axisX(CTRL_ROLL, PinRef(PA0), /*reverse=*/false, 0, 65535,
                               /*hysteresis=*/32, /*ewmaShift=*/4, /*pollMs=*/2);

void setup() { PanelGroup::setup(); }
void loop()  { PanelGroup::loop(); }   // polls the input, drains CAN — nothing else needed
```

---

## Implementation Notes

### Read + scale

```cpp
uint16_t AnalogInput::readScaled() {
    uint16_t raw = _pin.readAnalog();                    // 16-bit (STM32 ×16 / ADS1115 ×2)
    if (raw < _minRaw) raw = _minRaw; else if (raw > _maxRaw) raw = _maxRaw;
    uint32_t span   = (uint32_t)_maxRaw - _minRaw;       // own scale, not Arduino map():
    uint16_t scaled = span ? (uint16_t)((uint32_t)(raw - _minRaw) * 65535u / span) : 0;
    return _reverse ? (uint16_t)(65535u - scaled) : scaled;
}
```

`(raw - minRaw)` ≤ 65535, so `× 65535` ≤ 4.29e9 — fits `uint32`. Arduino `map()` is **not** used:
its intermediate `value * (toHigh - toLow)` overflows `long` for full-range 16-bit inputs.

### Integer EWMA + emit gate

```cpp
void AnalogInput::sample() {
    uint16_t scaled = readScaled();
    _acc     += (int32_t)scaled - (_acc >> _ewmaShift);  // α = 1/2^ewmaShift
    _smoothed = (uint16_t)(_acc >> _ewmaShift);
    if (shouldEmit(_smoothed)) emit(_smoothed);
}

bool AnalogInput::shouldEmit(uint16_t v) const {         // ports DcsBios PotentiometerEWMA
    return (_lastSent > v && (uint16_t)(_lastSent - v) > _hysteresis)
        || (v > _lastSent && (uint16_t)(v - _lastSent) > _hysteresis)
        || (v > (uint16_t)(65535u - _hysteresis) && v > _lastSent)   // top rail, moving up
        || (v < _hysteresis && v < _lastSent);                       // bottom rail, moving down
}
```

`_acc` holds the smoothed value `<< ewmaShift`; in steady state `_acc = scaled << ewmaShift`, so
`_smoothed == scaled`. `forceReport()` seeds `_acc = scaled << ewmaShift` so the boot baseline is the
actual reading (no ramp-from-zero), then emits unconditionally. `poll()` throttles reads to the
instance's `pollMs` and delegates to `sample()`.

### Read throttle, and the τ coupling

`poll()` re-reads at most every `_pollMs` ms — a constructor parameter, not a class constant, so two
AnalogInputs on one node read at different rates. Verified on the assembled PanelGroup Rev 1
(2026-08-14): PA0 at 1 ms and PA1 at 8 ms simultaneously, both holding target (997 reads/s and
125 reads/s), neither starving the other at an 8× spread. `PanelGroup::loop()` calls `poll()` on
every input unconditionally, every iteration, and owns no analog timer of its own.

**`pollMs` and `ewmaShift` are coupled: τ ≈ 2^`ewmaShift` × `pollMs`.** The defaults give
2^3 × 8 = 64 ms. Change one without the other and the filter changes character, not just the rate —
`ewmaShift = 4` at the default 8 ms poll gives τ = 128 ms, which is unusable on a flight axis.

**Validated HID stick axis** (assembled PanelGroup Rev 1, 2026-08-14) — τ = 2^4 × 2 = 32 ms:

```cpp
OpenSkyhawk::AnalogInput axisX(CTRL_ROLL, PIN_X, /*reverse=*/false, 0, 65535,
                               /*hysteresis=*/32, /*ewmaShift=*/4, /*pollMs=*/2);
```

Three preconditions, each of which binds:

- **1 kΩ + 100 nF RC at the MCU end of the analog input.** With it, both axes emitted zero times in
  100 consecutive seconds at rest. Removing it doubled that channel's raw noise (48 → 96), which is
  well over a hysteresis of 32 — the axis chatters. The base-board design record already specifies
  1 kΩ + 100 nF per variant.
- **`setDebug(false)`.** `emit()` blocks on a ~20-char USART1 write at 115200; with debug on the
  chain caps near 285 Hz, below the 500 Hz this configuration is chosen for.
- **HID routing.** See the note below on DCS-BIOS-routed values.

2 ms is not arbitrary: SimGateway declares its HID endpoint with `interval_ms = 2`
(`SimGateway.cpp:78-79`), so 500 Hz is exactly the USB report rate and a faster axis is discarded at
the USB boundary. The gain is **latency, not throughput** — real flying sits at 40–200 Hz per axis,
below even the shipped 125 Hz, so a faster poll buys up to ~6 ms less sampling delay on every
movement rather than more updates.

**`pollMs < 8` is a HID-axis setting.** A DCS-BIOS-routed value crosses PanelBridge as an ASCII line
on the shared 250000-baud host UART (~1250 lines/s for the whole cockpit); one pot at 500 Hz would
consume roughly 40% of that budget alone. Leave DCS-BIOS-routed pots at the default.

**`pollMs` is not clamped.** 0 means "read every loop iteration" — `now - _lastReadMs < 0` is never
true for unsigned — which is legal but makes τ track the loop rate, not a controlled quantity.
Contrast `ewmaShift`, which *is* clamped: an overflowed accumulator emits **wrong values**, while a
small `pollMs` only makes a poor choice. Different failure classes, different treatment.

**Still open — the 1.5 m harness.** All of the above used ~6" leads. Worst-case smoothed p-p peaked
at 24 against a threshold of 32, so it would not take much added pickup to push chatter back in.
Re-run the at-rest emission sweep before locking `hysteresis = 32` into a panel sketch.

**ADC source resolution:** `PinRef::readAnalog()` returns a 16-bit value, but the STM32F103 ADC is
**12-bit** hardware — `STM32Board::begin()` calls `analogReadResolution(16)` and the STM32duino
framework left-shifts the reading (×16) to fill the `uint16_t`. A GPIO input therefore carries 16-bit
*format* with 12-bit *real* resolution and tops at **65520** (low 4 bits always 0). The near-rail
force-send still reaches the rail (65520 > 65535 − `hysteresis`); for an exact 0–65535 span set
`maxRaw = 65520`. 12-bit is ample for the pot inputs this class serves (ARC-51 VOL). (ADS1115 channels
are a true 15-bit single-ended read ×2 → 0–65534.)

**FirmwarePlan reconciliation:** `FirmwarePlan/05` and the earlier stub specified a plain "32-count
dead-band" with no smoothing. This implementation instead ports the **DcsBios `PotentiometerEWMA`**
recipe (EWMA + hysteresis + near-rail), because the ×16 STM32 scaling amplifies ADC noise more than
DcsBios's 10-bit path; the filter parameters are configurable (defaulting to DcsBios-equivalent
values). `FirmwarePlan/05` is updated to this approach in this PR. `ewmaShift` is capped at
`MAX_EWMA_SHIFT` (15) — beyond that the int32 accumulator `scaled << ewmaShift` overflows at full
scale; the constructor clamps it (covered by `test_shift_bounds`).

### EVT transmission

`CANProtocol::sendBatched(canIdEvt(NODE_ID), {controlId, value})` + `[ANA]` debug line. PanelBridge
dispatches DCS-BIOS-routed values as `MULTIPOS` (`itoa(value)` → `set_state`); HID-routed values feed
a `HIDAxis`. No rescaling at either destination — the 16-bit value is used as-is.

---

## Dependencies

| Dependency | Source | Notes |
|---|---|---|
| PanelGroup | `Firmware/Libraries/PanelGroup` | InputBase, PinRef (analog read) |
| CANProtocol | `Firmware/Libraries/CANProtocol` | ControlPacket, sendBatched, canIdEvt |
| STM32Board | `Firmware/Libraries/STM32Board` | diagSerial (debug line) |
