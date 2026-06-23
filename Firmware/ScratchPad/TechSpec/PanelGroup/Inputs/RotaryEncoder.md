# RotaryEncoder — Technical Specification

**Status:** Done (compile-gated; hardware bench pending)
**FirmwarePlan ref:** `FirmwarePlan/05-panelgroup-api.md#rotaryencoder-new`
**Depends on:** `PinRef.md`, `PanelGroup.md`

---

## Responsibility

Incremental quadrature rotary encoder on two pins (A/B). Emits a **direction** per detent over CAN
(`ENCODER` dispatch): **1 = clockwise, 0 = counter-clockwise**. A direct `InputBase` subclass — a
*relative* control: it reports motion, not an absolute position.

Handles:
- **quadrature decode** — reads the 2-bit Gray state `(A<<1)|B`; a transition table accumulates a
  signed delta (ports DcsBios `RotaryEncoder`);
- **detent scaling** — emits a click only when `|delta| >= stepsPerDetent`, then reduces the delta
  by one detent. `stepsPerDetent` (1 / 2 / 4 / 8) matches the encoder's transitions-per-detent so
  one physical click = one EVT; it also rejects sub-detent jitter;
- **direction** — CW (1) / CCW (0); swap A/B to reverse the sensed direction.

Does **not** debounce on a timer (the quadrature table + detent divisor reject glitches), interpret
`controlId`, or enable internal pull-ups. `forceReport()` resyncs state and emits **nothing** — a
relative control has no baseline.

---

## File Layout

```
Firmware/Libraries/PanelGroup/Inputs/RotaryEncoder/
├── RotaryEncoder.h
└── RotaryEncoder.cpp
```

### Test project — `Firmware/Tests/RotaryEncoder/`

Self-contained — **no encoder hardware**. `debugSeed(state)` sets the starting Gray state and
`debugStep(ab)` feeds the next 2-bit A/B state through the decoder; assertions are on
`emitCount()` / `lastDir()` (`#ifdef ROTARYENCODER_TEST`). CAN runs in **normal mode** so the node
ACKs the PanelBridge. No jumpers or encoder needed (a real detented encoder on A/B is an optional
throughput sanity check on the bench).

| Scenario env | Verifies |
|---|---|
| `test_cw_detent` | CW Gray cycle 0→1→3→2→0 → one CW (1) EVT at FOUR_STEPS_PER_DETENT |
| `test_ccw_detent` | CCW cycle 0→2→3→1→0 → one CCW (0) EVT |
| `test_steps_per_detent` | ONE_STEP_PER_DETENT → every transition emits (4 per cycle) |
| `test_partial_no_emit` | 2 of 4 transitions → delta held, no EVT |
| `test_bounce` | jitter within a detent (4-step) → no spurious EVT |
| `test_reversal` | CW detent then CCW detent → 1 then 0, no stuck state |
| `test_force_report_noop` | `forceReport()` emits nothing; a later transition still emits |

---

## Public API

```cpp
enum StepsPerDetent : uint8_t {
    ONE_STEP_PER_DETENT = 1, TWO_STEPS_PER_DETENT = 2,
    FOUR_STEPS_PER_DETENT = 4, EIGHT_STEPS_PER_DETENT = 8,
};

class RotaryEncoder : public InputBase {
public:
    RotaryEncoder(uint16_t controlId, PinRef pinA, PinRef pinB,
                  StepsPerDetent stepsPerDetent = ONE_STEP_PER_DETENT);
    void poll() override;          // decode + emit on detent
    void forceReport() override;   // resync state, NO EVT (relative)
    void configure() override;
protected:
    uint8_t readState();           // (pinA << 1) | pinB
private:
    void decode(uint8_t state);    // transition table → delta → emit
    void emit(uint16_t dir);
    // _pinA, _pinB, _stepsPerDetent, _lastState, _delta, _initialized
};
```

The `StepsPerDetent` enum is in namespace `OpenSkyhawk`; reference it as
`OpenSkyhawk::FOUR_STEPS_PER_DETENT` (or `using namespace OpenSkyhawk;`).

---

## Sketch Usage

```cpp
#include <Wire.h>
#include <MCP23017.h>
#include <PanelGroup.h>
#include <Inputs/RotaryEncoder/RotaryEncoder.h>
#include <A4EC_CmdIds.h>

MCP23017 exp1(0x21, Wire);

// AN/ASN-41 present-position latitude knob (variable_step, multiturn). Detented encoder on two
// MCP23017 bits; 4 transitions per detent.
OpenSkyhawk::RotaryEncoder ppLatKnb(DCSIN_PPOS_LAT_KNB,
                                    PinRef(exp1, PORT_A, 0),
                                    PinRef(exp1, PORT_A, 1),
                                    OpenSkyhawk::FOUR_STEPS_PER_DETENT);

void setup() {
    Wire.begin();
    PanelGroup::registerExpander(exp1, PB8, PB9);   // INT pins on I2C2
    PanelGroup::setup();
}

void loop() {
    PanelGroup::loop();   // polls every input, drains CAN — RotaryEncoder needs nothing else
}
```

---

## Implementation Notes

### Quadrature decode

```cpp
void RotaryEncoder::decode(uint8_t state) {
    switch (_lastState) {                              // ported verbatim from DcsBios Encoders.h
        case 0: if (state == 2) _delta--; if (state == 1) _delta++; break;
        case 1: if (state == 0) _delta--; if (state == 3) _delta++; break;
        case 2: if (state == 3) _delta--; if (state == 0) _delta++; break;
        case 3: if (state == 1) _delta--; if (state == 2) _delta++; break;
    }
    _lastState = state;
    if (_delta >=  (int8_t)_stepsPerDetent) { emit(1); _delta -= _stepsPerDetent; }   // CW
    if (_delta <= -(int8_t)_stepsPerDetent) { emit(0); _delta += _stepsPerDetent; }   // CCW
}
```

`poll()` calls `decode(readState())`. The Gray state is `(A<<1)|B` ∈ 0..3. Valid single-step
transitions move the delta ±1; double/invalid jumps are ignored by the table. The `stepsPerDetent`
divisor both scales clicks to the encoder and rejects sub-detent jitter (the `test_bounce` case).

### forceReport

`forceReport()` reads the current Gray state into `_lastState` (so the first `poll()` sees no
spurious transition), zeroes the delta, sets `_initialized`, and **emits no EVT**. Unlike the switch
and analog inputs, a relative encoder has no absolute position to broadcast at boot / SYNC_REQ.

### Reading over MCP23017

The encoders on AN/ASN-41 + AN/ARC-51A sit on MCP23017s read through the bus INT (PinRef returns the
cached port bit, refreshed on the expander interrupt). The detent period at human turn speeds (≥ ~10
ms) is well within the INT-refresh latency; the **11-encoder throughput** on one node is the open
item flagged on the controller — verify at B6.

### EVT transmission & dispatch

`CANProtocol::sendBatched(canIdEvt(NODE_ID), {controlId, dir})` + `[ENC]` debug line. PanelBridge's
**`ENCODER` dispatch is already wired**; it maps `dir` 0/1 to the control's DCS-BIOS argument strings
(`"DEC"`/`"INC"` for `fixed_step`; the ± increment for `variable_step`) from the input-map entry —
so the **same class drives both kinds**, the difference is entirely in the map.

> **Deferred (panel-build, not this class):** `tools/gen_a4ec/gen_a4ec.py` currently classifies the
> A-4E-C encoder controls (`ARC51_FREQ_*` as `set_state`, the `*_KNB` knobs as `variable_step`) as
> `MULTIPOS` / `ANALOG`. They need a curated override to emit `ENCODER` + the dec/inc args so they
> dispatch correctly end-to-end. This rides with the ASN-41 / ARC-51 panel build (B6), where the
> mixed-input mapping is exercised against live DCS-BIOS.

---

## Dependencies

| Dependency | Source | Notes |
|---|---|---|
| PanelGroup | `Firmware/Libraries/PanelGroup` | InputBase, PinRef |
| CANProtocol | `Firmware/Libraries/CANProtocol` | ControlPacket, sendBatched, canIdEvt |
| STM32Board | `Firmware/Libraries/STM32Board` | diagSerial (debug line) |
