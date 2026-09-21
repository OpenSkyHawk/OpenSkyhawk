# RotaryAcceleratedEncoder — Technical Specification

**Status:** Ready for implementation (#287) — `RotaryEncoder` family member (D16)
**FirmwarePlan ref:** `FirmwarePlan/05-panelgroup-api.md` (RotaryAcceleratedEncoder), `FirmwarePlan/00-decisions.md` (D16; supersedes D9)
**Depends on:** `RotaryEncoder.md`

---

## Responsibility

`RotaryAcceleratedEncoder : RotaryEncoder` — the `DcsBios::RotaryAcceleratedEncoder` equivalent
(`src/internal/Encoders.h` in dcs-bios-arduino-library). It adds two behaviours to the plain
encoder, both on by construction, exactly as in DCS-BIOS:

1. **Momentum filter** — the DCS-BIOS class's stated purpose ("noisy/faulty rotaries"). A signed
   momentum counter builds by one per transition in the current direction, up to
   ±(`MAX_MOMENTUM` × stepsPerDetent) with `MAX_MOMENTUM` = 4. A transition **against** non-zero
   momentum is dropped (not added to the delta) and only moves momentum one step toward zero. When
   no detent has fired for `STOPPED_THRESHOLD_MS` = 500 ms, momentum resets to zero.
2. **Speed** — a detent that completes less than `FAST_THRESHOLD_MS` = 175 ms after the previous
   one contributes `fastStep` instead of `step`. Two-tier, not a ramp.

Speed only applies in **REL** mode (`variable_step`): the DIR wire is strictly ±1 — PanelBridge
drops anything else — so a DIR-mode instance gets the momentum filter only. That matches how
DCS-BIOS users apply the class to INC/DEC controls (`"INC","DEC","INC","DEC"`).

Does **not** change the wire format, PanelBridge or the input map (supersedes D9): a fast detent is
just a larger `±step` on `canIdEvtRel`. Does **not** alter the plain `RotaryEncoder` — its public
constructor stays filter-free.

---

## File Layout

```
Firmware/Libraries/PanelGroup/
└── Inputs/RotaryEncoder/
    ├── RotaryEncoder.{h,cpp}             ← gains a protected constructor + the filter/speed path
    └── RotaryAcceleratedEncoder.h        ← constructors only (header-only subclass)
```

### Test project

`Firmware/Tests/RotaryEncoder/` — the eight existing envs stay untouched (plain encoder). New envs,
driven through the existing `debugSeed()` / `debugStep()` seams with real `delay()` for timing (as
the ActionButton tests do), so any STM32 node runs them with no wiring:

| Env | Asserts |
|---|---|
| `accel_reverse_blip` | a single reverse transition mid-spin is swallowed; no reverse detent |
| `accel_momentum_reset` | after 500 ms idle, a genuine reversal registers normally |
| `accel_fast_slow` | detents < 175 ms apart emit `fastStep`, ≥ 175 ms emit `step` |
| `accel_burst_coalesce` | a mixed slow + fast burst drains as one REL frame whose value is the sum |
| `accel_dir_filter_only` | DIR mode: the filter applies; every emit is exactly ±1 |

---

## Public API

```cpp
// RotaryAcceleratedEncoder.h
#pragma once
#include "RotaryEncoder.h"

namespace OpenSkyhawk {

class RotaryAcceleratedEncoder : public RotaryEncoder {
public:
    /** REL (variable_step): momentum filter + speed. fastStep is required. */
    RotaryAcceleratedEncoder(uint16_t controlId, PinRef pinA, PinRef pinB,
                             EncoderStepsPerDetent stepsPerDetent,
                             int16_t step, int16_t fastStep)
        : RotaryEncoder(controlId, pinA, pinB, stepsPerDetent, EncoderMode::Rel, step,
                        /*momentumFilter*/ true, fastStep) {}

    /** DIR (fixed_step): momentum filter only — the DIR wire carries ±1, so there is no speed. */
    RotaryAcceleratedEncoder(uint16_t controlId, PinRef pinA, PinRef pinB,
                             EncoderStepsPerDetent stepsPerDetent, EncoderMode mode /* Dir */)
        : RotaryEncoder(controlId, pinA, pinB, stepsPerDetent, mode, DEFAULT_STEP,
                        /*momentumFilter*/ true, /*fastStep*/ 0) {}
};

} // namespace OpenSkyhawk
```

The DIR constructor rejects `EncoderMode::Rel` (debug assertion) — REL users take the first
constructor, which makes `fastStep` explicit.

Base-class additions (`RotaryEncoder.h`), all `protected` so the plain encoder's public API is
unchanged:

```cpp
protected:
    RotaryEncoder(uint16_t controlId, PinRef pinA, PinRef pinB,
                  EncoderStepsPerDetent stepsPerDetent, EncoderMode mode, int16_t step,
                  bool momentumFilter, int16_t fastStep);   // fastStep 0 = speed off

    static constexpr uint8_t  MAX_MOMENTUM         = 4;
    static constexpr uint32_t FAST_THRESHOLD_MS    = 175;
    static constexpr uint32_t STOPPED_THRESHOLD_MS = 500;
```

---

## Implementation Notes

### Where the filter goes — `decode()`

Today the transition switch writes `_delta` directly. It becomes: switch → `int8_t dir` → momentum
gate (when enabled) → `_delta += dir` — the DCS-BIOS order. With the filter off the result is
byte-identical to today, which is what keeps the existing envs valid.

### Where speed is classified — also `decode()`

`decode()` runs wherever sampling happens — `poll()` at loop rate, or `sampleTick()` in the
ShiftBus timer ISR (`SHIFTBUS_ISR_HZ`). Speed must be classified there, per detent, using
`millis()` (safe in the ISR on STM32). Classifying at drain time would be wrong: a loop stalled by
an OLED flush drains several detents at once and their spacing would be lost.

So in REL mode the pending counter holds a **step magnitude** rather than a detent count: each
completed detent adds `step` or `fastStep`. `drainPending()` chunks the magnitude into `int16`
frames as it does now. DIR keeps the detent count and its cap of 8.

### Why this is safe for existing sketches

The public constructor, `EncoderMode`, the frames and every existing env are unchanged. A sketch
opts in only by naming `RotaryAcceleratedEncoder`.

---

## Dependencies

| Dependency | Source | Notes |
|---|---|---|
| RotaryEncoder | `Inputs/RotaryEncoder` | family base — decode, drain, emit |
| CANProtocol | `Firmware/Libraries/CANProtocol` | `canIdEvtRel` / `canIdEvtDir` via the base |
