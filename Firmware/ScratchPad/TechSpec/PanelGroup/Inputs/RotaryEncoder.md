# RotaryEncoder — Technical Specification

**Status:** Done — dual-mode REL/DIR (#147), verified end to end against live DCS on the `E2E_DCS_Test` node (PR #160: REL `DEST_LAT_KNB` ±3200, DIR `ARC51_FREQ_10MHZ` INC/DEC). Family base for `RotaryAcceleratedEncoder` (D16). (Prior single-mode 0/1 build: hardware-verified 2026-06-23.)
**FirmwarePlan ref:** `FirmwarePlan/05-panelgroup-api.md` (RotaryEncoder)
**Depends on:** `PinRef.md`, `PanelGroup.md`

---

## Responsibility

Incremental quadrature rotary encoder on two pins (A/B). Emits a **signed relative value** per detent
over CAN — direction in the sign, magnitude set by the **mode**. A direct `InputBase` subclass — a
*relative* control: it reports motion, not an absolute position.

Two modes (`EncoderMode`, chosen at construction):
- **REL** (`variable_step` knob — ASN-41 nav, altimeter): emits `±step` (default 3200) on
  `canIdEvtRel`; the bridge formats `%+d`. `step` is build-side feel and lives on the node.
- **DIR** (`fixed_step` selector with no indicator — ARC-51 freq): emits `±1` on `canIdEvtDir`; the
  bridge formats `INC`/`DEC`. Stateless — DCS owns the position and clamps at the band edges.

Handles:
- **quadrature decode** — reads the 2-bit Gray state `(A<<1)|B`; a transition table accumulates a
  signed delta (ports DcsBios `RotaryEncoder`);
- **detent scaling** — emits a click only when `|delta| >= stepsPerDetent`, then reduces the delta
  by one detent. `stepsPerDetent` (1 / 2 / 4 / 8) matches the encoder's transitions-per-detent so
  one physical click = one EVT; it also rejects sub-detent jitter;
- **direction** — CW positive / CCW negative; swap A/B to reverse the sensed direction.

Does **not** debounce on a timer (the quadrature table + detent divisor reject glitches), interpret
`controlId`, or enable internal pull-ups. `forceReport()` resyncs state and emits **nothing** — a
relative control has no baseline, so **both modes are preset-safe** (never clobber a mission preset
at boot / SYNC).

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
`emitCount()` / `lastValue()` / `lastFrame()` (`#ifdef ROTARYENCODER_TEST`). CAN runs in **silent
loopback** (`CANProtocol::startLoopback()`, as the ActionButton tests do), so the board runs **alone** —
no bus, no PanelBridge, no jumpers, no encoder (a real detented encoder on A/B is an optional
throughput sanity check on the bench). Don't run these on a board wired to a live bus: a loopback node
never ACKs and would drive the bridge error-passive. (Switched from normal mode in #287.)

| Scenario env | Verifies |
|---|---|
| `test_cw_detent` | CW cycle 0→1→3→2→0 → one REL EVT (`+step`) on `canIdEvtRel` at FOUR_STEPS |
| `test_ccw_detent` | CCW cycle 0→2→3→1→0 → one REL EVT (`-step`) |
| `test_dir_mode` | DIR mode: CW → `+1` on `canIdEvtDir`; CCW → `-1` |
| `test_steps_per_detent` | EncoderStepsPerDetent::One → every transition emits (4 per cycle) |
| `test_partial_no_emit` | 2 of 4 transitions → delta held, no EVT |
| `test_bounce` | jitter within a detent (4-step) → no spurious EVT |
| `test_reversal` | CW detent then CCW detent → `+step` then `-step`, no stuck state |
| `test_force_report_noop` | `forceReport()` emits nothing; a later transition still emits |

---

## Public API

```cpp
enum class EncoderStepsPerDetent : uint8_t { One = 1, Two = 2, Four = 4, Eight = 8 };
enum class EncoderMode : uint8_t { Rel, Dir };   // Rel: ±step → %+d.  Dir: ±1 → INC/DEC.

class RotaryEncoder : public InputBase {
public:
    static constexpr int16_t DEFAULT_STEP = 3200;  // DCS suggested_step (~20 detents/full throw)
    RotaryEncoder(uint16_t controlId, PinRef pinA, PinRef pinB,
                  EncoderStepsPerDetent stepsPerDetent = EncoderStepsPerDetent::One,
                  EncoderMode mode = EncoderMode::Rel, int16_t step = DEFAULT_STEP);
    void poll() override;          // decode + emit on detent
    void forceReport() override;   // resync state, NO EVT (relative)
    void configure() override;
protected:
    uint8_t readState();           // (pinA << 1) | pinB
private:
    void decode(uint8_t state);    // transition table → delta → emit
    void emit(int8_t dir);         // dir = +1 (CW) / -1 (CCW); mode picks frame + value
    // _pinA, _pinB, _mode, _step, _stepsPerDetent, _lastState, _delta, _initialized
};
```

The `EncoderStepsPerDetent` + `EncoderMode` enums are scoped (`enum class`) in namespace
`OpenSkyhawk`; reference them as `OpenSkyhawk::EncoderStepsPerDetent::Four` /
`OpenSkyhawk::EncoderMode::Dir` (or `using namespace OpenSkyhawk;`). The ctor keeps `stepsPerDetent`
4th (a hardware property always worth setting), so a REL knob just sets that; add `EncoderMode::Dir`
(and an optional `step`) for the selector case.

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
                                    OpenSkyhawk::EncoderStepsPerDetent::Four);

void setup() {
    Wire.begin();
    PanelGroup::registerExpander(exp1, PB12, PB13); // INT pins (PB8/PB9 = ShiftBus LOAD/LATCH)
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
    int8_t dir = 0;
    switch (_lastState) {                              // ported verbatim from DcsBios Encoders.h
        case 0: if (state == 2) dir = -1; if (state == 1) dir = +1; break;
        case 1: if (state == 0) dir = -1; if (state == 3) dir = +1; break;
        case 2: if (state == 3) dir = -1; if (state == 0) dir = +1; break;
        case 3: if (state == 1) dir = -1; if (state == 2) dir = +1; break;
    }
    _lastState = state;
    if (_filter) { /* momentum gate — RotaryAcceleratedEncoder only, see its TechSpec */ }
    _delta += dir;
    if (_delta >=  (int8_t)_stepsPerDetent) { countDetent(+1); _delta -= _stepsPerDetent; }  // CW
    if (_delta <= -(int8_t)_stepsPerDetent) { countDetent(-1); _delta += _stepsPerDetent; }  // CCW
}
```

`poll()` calls `decode(readState())` (or the ShiftBus sampler calls it via `sampleTick()`). The Gray
state is `(A<<1)|B` ∈ 0..3. Valid single-step transitions move the delta ±1; double/invalid jumps are
ignored by the table. The `stepsPerDetent` divisor both scales clicks to the encoder and rejects
sub-detent jitter (the `test_bounce` case). `countDetent()` bumps the pending counter that
`drainPending()` turns into EVTs loop-side.

**Family base (#287, D16).** A protected constructor adds two switches used only by
`RotaryAcceleratedEncoder`: `momentumFilter` (the DCS-BIOS momentum gate between the table and
`_delta`) and `fastStep` (per-detent speed classification into a separate `_pendingFast`). The public
constructor passes `false` / `0`, and with both off every branch reduces to the original code — no
`millis()` read, same pending counter, same REL whole-detent chunking — so the eight envs below are
unchanged. Test seams `debugDecode()` / `debugDrain()` (decode or drain alone) were added for the
subclass's burst test.

### forceReport

`forceReport()` reads the current Gray state into `_lastState` (so the first `poll()` sees no
spurious transition), zeroes the delta and both pending counters (and the momentum / last-detent state the subclass uses), sets `_initialized`, and **emits no EVT**. Unlike the switch
and analog inputs, a relative encoder has no absolute position to broadcast at boot / SYNC_REQ.

### Reading over MCP23017

The encoders on AN/ASN-41 + AN/ARC-51A sit on MCP23017s read through the bus INT (PinRef returns the
cached port bit, refreshed on the expander interrupt). The detent period at human turn speeds (≥ ~10
ms) is well within the INT-refresh latency; the **11-encoder throughput** on one node is the open
item flagged on the controller — verify at B6.

### EVT transmission & dispatch

`emit(dir)` (dir = +1 CW / −1 CCW) computes the value + frame from the mode and calls
`CANProtocol::sendBatched(frame, {controlId, (uint16_t)value})` + an `[ENC]` debug line:

| mode | value | frame | bridge formats | DCS interface |
|---|---|---|---|---|
| REL | `±step` (e.g. ±3200) | `canIdEvtRel(NODE_ID)` | `%+d` | `variable_step` |
| DIR | `±1` | `canIdEvtDir(NODE_ID)` | `INC`/`DEC` | `fixed_step` |

The dispatch **form is the CAN frame**, not an input-map field — the bridge reads the payload as
`int16` and formats by which frame it arrived on (#147). The generated map carries only
`controlId → name`; there are no per-control dec/inc args, and the REL step magnitude lives entirely
on the node (this ctor) so retuning a knob's feel needs only a node reflash, never a bridge rebuild.

> **Sequencing:** the bridge REL/DIR dispatch + the map collapse land in #147's bridge PR. Until
> then the bridge drops `canIdEvtRel`/`canIdEvtDir` frames — harmless, no encoder controller is
> assembled yet.

---

## Dependencies

| Dependency | Source | Notes |
|---|---|---|
| PanelGroup | `Firmware/Libraries/PanelGroup` | InputBase, PinRef |
| CANProtocol | `Firmware/Libraries/CANProtocol` | ControlPacket, sendBatched, canIdEvtRel/canIdEvtDir |
| STM32Board | `Firmware/Libraries/STM32Board` | diagSerial (debug line) |
