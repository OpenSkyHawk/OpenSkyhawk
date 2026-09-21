# RotaryAcceleratedEncoder — Technical Specification

**Status:** Implemented (#287) — builds clean; hardware-verify of the 5 envs pending. `RotaryEncoder` family member (D16)
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
   one contributes `fastStep` instead of `step`. Two-tier, not a ramp. `fastStep` uses the same sign
   convention as `step` (a negative `step` flips direction — so should `fastStep`); `fastStep = 0`
   turns the speed-up off. **The first detent after a resync (`forceReport()`) is always slow** —
   DCS-BIOS times it from construction, which can misclassify it.

Speed only applies in **REL** mode (`variable_step`): the DIR wire is strictly ±1 — PanelBridge
drops anything else — so a DIR-mode instance gets the momentum filter only. That matches how
DCS-BIOS users apply the class to INC/DEC controls (`"INC","DEC","INC","DEC"`).

Does **not** change the wire format, PanelBridge or the input map (supersedes D9): a fast detent is
just a larger `±step` on `canIdEvtRel`. Does **not** alter the plain `RotaryEncoder` — its public
constructor stays filter-free.

---

## File Layout

Own folder and own test project, like every other class (the `MultiPosInput` family precedent):

```
Firmware/Libraries/PanelGroup/
├── Inputs/RotaryEncoder/RotaryEncoder.{h,cpp}                 ← family base: protected ctor + filter/speed path
└── Inputs/RotaryAcceleratedEncoder/RotaryAcceleratedEncoder.h ← constructors only (header-only)
```

Included from `OpenSkyhawk.h`.

### Test project — `Firmware/Tests/RotaryAcceleratedEncoder/`

Same `env_base` as `Firmware/Tests/RotaryEncoder` (`-DROTARYENCODER_TEST`). Driven through the base's
test seams — `debugSeed()`, `debugStep()` (decode + drain), and `debugDecode()` / `debugDrain()`
(added for this class, to build up a pending burst) — with real `delay()` for timing. **Rig:** the
STM32 on the CAN bus with a PanelBridge that ACKs (as for the plain encoder's envs); no encoder
hardware.

| Env | Asserts |
|---|---|
| `test_reverse_blip` | a single reverse transition mid-spin is swallowed; the same sequence costs a plain `RotaryEncoder` a detent |
| `test_momentum_reset` | an immediate reversal is eaten; after 500 ms idle a reversal registers in full |
| `test_fast_slow` | first detent slow; < 175 ms apart → `fastStep`; ≥ 175 ms → `step` |
| `test_burst_coalesce` | slow + fast detents drain as one REL frame = their sum; a sum beyond int16 splits into frames that add up (32767 + 5633) |
| `test_dir_filter_only` | DIR: every emit is exactly ±1; the filter still swallows a blip; reversal from rest → −1 |

The eight `Firmware/Tests/RotaryEncoder` envs are unchanged and still pass — the plain path is
byte-identical.

---

## Public API

```cpp
// RotaryAcceleratedEncoder.h
#pragma once
#include <Inputs/RotaryEncoder/RotaryEncoder.h>

namespace OpenSkyhawk {

class RotaryAcceleratedEncoder : public RotaryEncoder {
public:
    /** REL (variable_step): momentum filter + speed. fastStep is required. */
    RotaryAcceleratedEncoder(uint16_t controlId, PinRef pinA, PinRef pinB,
                             EncoderStepsPerDetent stepsPerDetent,
                             int16_t step, int16_t fastStep)
        : RotaryEncoder(controlId, pinA, pinB, stepsPerDetent, EncoderMode::Rel, step,
                        /*momentumFilter*/ true, fastStep) {}

    /** DIR (fixed_step): momentum filter only — the DIR wire carries ±1, so there is no speed.
     *  Passing EncoderMode::Rel here gives a REL knob with the filter and no speed-up. */
    RotaryAcceleratedEncoder(uint16_t controlId, PinRef pinA, PinRef pinB,
                             EncoderStepsPerDetent stepsPerDetent, EncoderMode mode /* Dir */)
        : RotaryEncoder(controlId, pinA, pinB, stepsPerDetent, mode, DEFAULT_STEP,
                        /*momentumFilter*/ true, /*fastStep*/ 0) {}
};

} // namespace OpenSkyhawk
```

The mode-taking constructor is meant for `EncoderMode::Dir`; given `EncoderMode::Rel` it yields a REL
knob with the filter and no speed-up (static-init time is too early to log, so it is documented
rather than asserted).

Base-class additions (`RotaryEncoder.h`). The family constructor is `protected`; the three timing
constants are `public` (tests and sketches read them). Nothing a plain-encoder sketch uses changed:

```cpp
protected:
    RotaryEncoder(uint16_t controlId, PinRef pinA, PinRef pinB,
                  EncoderStepsPerDetent stepsPerDetent, EncoderMode mode, int16_t step,
                  bool momentumFilter, int16_t fastStep);   // fastStep 0 = speed off

public:
    static constexpr int8_t   MAX_MOMENTUM         = 4;
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

Fast detents go to a **separate `volatile int8_t _pendingFast`** counter; `_pendingDetents` and its
cap logic are untouched. `drainPending()` reads and clears both under the existing `noInterrupts()`
section:

- **plain REL** (`_pendingFast == 0`, always true for `RotaryEncoder`) — the existing
  whole-detent chunking, byte-identical;
- **accelerated REL** — `slow × step + fast × fastStep` as one int32, split into ±32767 frames only
  if it overflows int16 (`variable_step` adds, so a split sums to the same result);
- **DIR** — unchanged; `fastStep` is forced to 0 in DIR, so `_pendingFast` is never used.

`millis()` is read only when the filter or the speed-up is on — the plain encoder never reads the
clock in `decode()`.

### Why this is safe for existing sketches

The public constructor, `EncoderMode`, the frames and every existing env are unchanged. A sketch
opts in only by naming `RotaryAcceleratedEncoder`.

---

## Dependencies

| Dependency | Source | Notes |
|---|---|---|
| RotaryEncoder | `Inputs/RotaryEncoder` | family base — decode, drain, emit |
| CANProtocol | `Firmware/Libraries/CANProtocol` | `canIdEvtRel` / `canIdEvtDir` via the base |
