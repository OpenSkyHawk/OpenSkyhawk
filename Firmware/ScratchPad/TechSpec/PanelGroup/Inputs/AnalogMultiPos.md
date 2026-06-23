# AnalogMultiPos — Technical Specification

**Status:** Done (compile-gated; hardware bench pending)
**FirmwarePlan ref:** `FirmwarePlan/05-panelgroup-api.md#analogmultipos-new`
**Depends on:** `MultiPosInput.md`, `PinRef.md`, `PanelGroup.md`

---

## Responsibility

Resistor-ladder multi-position selector — a single analog `PinRef` reads a different voltage per
position. Emits the resolved position index 0..N-1 over CAN (`MULTIPOS` dispatch). A subclass of
`MultiPosInput`: it provides only the analog **read** (mapping the 16-bit ADC value to a position
via detection bands); the base owns debounce / emit-on-change / hold-last / EVT transmission.

Handles:
- **explicit ladder** (`posVals[]`) or **equal-spacing shorthand** (N only);
- **`ANALOG_NC`** entries — a position with no detent, never emitted; its neighbours' bands span it;
- **deadband hysteresis** — a reading in the gap between two bands resolves to `NO_POSITION`, so the
  base holds the last position (no boundary flicker);
- an **8 ms ADC read throttle**.

Does **not** interpret `controlId`, debounce on a timer (the base window is 0), or own the emit path.

---

## File Layout

```
Firmware/Libraries/PanelGroup/Inputs/AnalogMultiPos/
├── AnalogMultiPos.h
└── AnalogMultiPos.cpp
```

### Test project — `Firmware/Tests/AnalogMultiPos/`

Self-contained — **no analog hardware**. The band logic is asserted via `debugResolve(raw)` and the
full poll path via `debugSetRaw(raw)` + `position()`/`emitCount()` (`#ifdef ANALOGMULTIPOS_TEST` /
`MULTIPOS_TEST` build flags). The integration scenarios run CAN in normal mode so the node ACKs the
PanelBridge.

| Scenario env | Verifies |
|---|---|
| `test_resolve` | detent values → index; band edges; deadband gaps → `NO_POSITION`; range ends |
| `test_analog_nc` | an `ANALOG_NC` position is never emitted; neighbours span its place |
| `test_equal_spacing` | shorthand ctor → evenly spaced bands |
| `test_force_report` | injected reading resolved + emitted on `forceReport()` |
| `test_change_emit` | emit-on-change; held reading no duplicate; deadband gap holds last |

---

## Public API

```cpp
namespace OpenSkyhawk {

static constexpr uint16_t ANALOG_NC = 0xFFFF;   // posVals[] sentinel — no detent at this position

class AnalogMultiPos : public MultiPosInput {
public:
    static constexpr uint16_t DEFAULT_DEADBAND = 1000;
    static constexpr uint16_t POLL_MS          = 8;

    // Explicit ladder. posVals = N expected 16-bit ADC values (ANALOG_NC for a no-detent position).
    AnalogMultiPos(uint16_t controlId, PinRef pin, uint8_t numPos,
                   const uint16_t* posVals, uint16_t deadband = DEFAULT_DEADBAND);
    // Equal-spacing shorthand — positions evenly spaced 0..65535.
    AnalogMultiPos(uint16_t controlId, PinRef pin, uint8_t numPos,
                   uint16_t deadband = DEFAULT_DEADBAND);

    void configure() override;
protected:
    uint16_t readRaw() override;
private:
    uint16_t resolve(uint16_t raw) const;
    uint16_t posValAt(uint8_t i) const;
    bool     isValid(uint8_t i) const;
    PinRef _pin; const uint16_t* _posVals; uint16_t _deadband, _cachedIdx; uint32_t _lastReadMs;
};

}
```

---

## Sketch Usage

```cpp
#include <PanelGroup.h>
#include <Inputs/AnalogMultiPos/AnalogMultiPos.h>
#include <A4EC_CmdIds.h>

// APN-153 doppler mode: OFF / STBY / LAND / SEA / TEST on one resistor ladder → STM32 ADC pin.
// Measure each detent's ADC value on the bench and fill in posVals.
const uint16_t DOPPLER_SEL_VALS[] = { 3300, 19700, 32800, 45900, 62200 };
OpenSkyhawk::AnalogMultiPos dopplerSel(DCSIN_DOPPLER_SEL, PinRef(PA1), 5, DOPPLER_SEL_VALS);

void setup() { PanelGroup::setup(); }
void loop()  { PanelGroup::loop(); }   // polls the input, drains CAN — nothing else needed
```

---

## Key Data Structures

`ANALOG_NC = 0xFFFF` — a `posVals[]` entry meaning "this position has no detent". It shares the
numeric value of `MultiPosInput::NO_POSITION` but is a *different concept* (an authoring sentinel in
the ladder, vs. the readRaw "nothing active" return). It is the `uint16_t` parallel of
`SwitchMultiPos`'s `PIN_NC` — same "no physical input at this index" role, but a `uint16_t`
sentinel rather than a `PinRef` because `posVals` holds ADC values; kept `== MultiPosInput::NO_POSITION`
so the family shares one sentinel value. The position array is caller-owned (or
`nullptr` for the shorthand); per-instance state otherwise.

---

## Implementation Notes

### Band resolve

```cpp
uint16_t AnalogMultiPos::resolve(uint16_t raw) const {
    for (uint8_t i = 0; i < _numPositions; i++) {
        if (!isValid(i)) continue;                       // skip ANALOG_NC positions
        int32_t lo = 0, hi = 65535;
        // nearest VALID neighbour each side → midpoint, trimmed by the deadband
        for (int16_t p = (int16_t)i - 1; p >= 0; p--)
            if (isValid(p)) { lo = ((int32_t)posValAt(p) + posValAt(i)) / 2 + _deadband; break; }
        for (uint8_t n = i + 1; n < _numPositions; n++)
            if (isValid(n)) { hi = ((int32_t)posValAt(i) + posValAt(n)) / 2 - _deadband; break; }
        if ((int32_t)raw >= lo && (int32_t)raw <= hi) return i;
    }
    return NO_POSITION;   // in a deadband gap → base holds the last position (hysteresis)
}
```

- **Band geometry:** position *i*'s band runs from the midpoint to its previous valid neighbour to
  the midpoint to its next valid neighbour, each edge pulled in by `deadband` counts. The outer
  positions reach 0 / 65535. Adjacent bands therefore leave a `2·deadband`-wide gap at each
  midpoint; a reading in the gap returns `NO_POSITION` and the base holds the last position — the
  hysteresis that stops flicker when the wiper sits near a boundary. Space detents > 2·deadband apart.
- **`ANALOG_NC`:** `isValid()` is false for those entries, so they are skipped in the scan *and* in
  the neighbour search — the index is reserved but never emitted, and its two valid neighbours' bands
  meet at their own shared midpoint.
- **Equal-spacing shorthand:** `posValAt(i)` computes `i · 65535 / (N-1)` when no `posVals` array
  was given; all positions valid.

### ADC throttle & debounce

`readRaw()` re-reads the ADC at most every `POLL_MS` (8 ms), caching the resolved index between
reads. `forceReport()` (boot burst, SYNC_REQ) **bypasses the throttle** and samples the current ADC —
it must report the present position, never a stale or uninitialised cache (at boot, before `millis()`
reaches `POLL_MS`, an un-bypassed throttle would emit position 0). The base debounce window is **0** —
the deadband gaps provide the filtering, so no timer is needed; emit-on-change still fires only when
the resolved index changes.

### EVT transmission

Inherited from `MultiPosInput` — `sendBatched(canIdEvt(NODE_ID), {controlId, index})`, routed by
PanelBridge as `MULTIPOS` (`itoa(index)`); SimGateway needs no declaration. Same dispatch as
`SwitchMultiPos`, so a selector can move between ladder and one-hot wiring with no DCS change.

---

## Dependencies

| Dependency | Source | Notes |
|---|---|---|
| MultiPosInput | `Firmware/Libraries/PanelGroup/Inputs/MultiPosInput` | base: debounce, emit-on-change, EVT |
| PanelGroup | `Firmware/Libraries/PanelGroup` | InputBase, PinRef (analog read) |
| CANProtocol | `Firmware/Libraries/CANProtocol` | ControlPacket, sendBatched (via base) |
