# SwitchMultiPos — Technical Specification

**Status:** Done (hardware-verified — 10/10 scenarios PASS 2026-06-22)
**FirmwarePlan ref:** `FirmwarePlan/05-panelgroup-api.md#switchmultipos-new`
**Depends on:** `MultiPosInput.md`, `PinRef.md`, `PanelGroup.md`

---

## Responsibility

Multi-position rotary selector — N discrete digital `PinRef`s, exactly one active at a time.
Emits the active position index 0..N-1 over CAN (`MULTIPOS` dispatch). A subclass of
`MultiPosInput`: it provides only the **read** (a one-hot pin scan); the base owns the debounce,
emit-on-change, hold-last, and EVT transmission.

Handles:
- **one-hot read** — the active pin reads LOW (`reverse = false`, default) or HIGH (`reverse =
  true`); its array index is the position;
- **`PIN_NC` detents** — a position with no physical pin (e.g. a sprung OFF). When no electrical
  pin is active, the `PIN_NC` slot's index is reported;
- **hold-last** — if no pin is active and there is no `PIN_NC` slot, the last confirmed position
  is held (delegated to the base via `NO_POSITION`).

Does **not** interpret `controlId`, debounce on its own (the base does), or enable internal
pull-ups (board wiring supplies bias).

---

## File Layout

```
Firmware/Libraries/PanelGroup/Inputs/SwitchMultiPos/
├── SwitchMultiPos.h
└── SwitchMultiPos.cpp
```

### Test project — `Firmware/Tests/SwitchMultiPos/`

GPIO-driven bench tests (mirror `Tests/Switch2Pos/`): control output pins drive the selector
input pins through jumpers; `check(label, ok)` → `STM32Board::diagSerial()` PASS/FAIL. The
resolved index is asserted directly via `SwitchMultiPos::position()` and the emit count via the
`#ifdef MULTIPOS_TEST` `emitCount()` seam — **not** a captured CAN frame, so there is no
loopback fragility. CAN runs in **normal mode** (`CANProtocol::start()`): the node ACKs the
(unmodified) PanelBridge and the EVTs reach it — drive a position, watch the selector move in DCS.
Default rig: jumper `PB0→PA0, PB1→PA1, PB4→PA4, PB5→PA5` (4 positions; `test_pin_nc` uses
`PB0→PA0, PB4→PA4`).

| Scenario env | Verifies |
|---|---|
| `test_configure` | `configure()` emits nothing; `poll()` is a no-op until `forceReport()` |
| `test_force_report` | boot read emits the current index once; repeats on a second call |
| `test_one_hot` | driving each pin active → its index 0..N-1; `controlId` forwarded |
| `test_hold_last` | all pins open → last index held, no spurious EVT |
| `test_pin_nc` | a `PIN_NC` slot → "no active" resolves to that detent index |
| `test_reverse` | `reverse = true` flips the active level |
| `test_debounce` | index must hold 20 ms before EVT; sub-window bounce emits nothing |
| `test_no_duplicate_evt` | a held position emits exactly once |
| `test_jump` | a non-adjacent jump (pos 1→3, 2 never read) emits exactly `3` |
| `test_fast_sweep` | sweeping intermediates < 20 ms emits only the settled position |

---

## Public API

```cpp
class SwitchMultiPos : public MultiPosInput {
public:
    static constexpr uint16_t DEBOUNCE_MS = 20;

    /**
     * @param controlId  DCSIN_* or CTRL_* constant. Determines PanelBridge routing.
     * @param pins       Caller-owned array of N PinRefs, one per position (must outlive this
     *                   object — define it static/global, like the sketch wiring map). Use
     *                   PIN_NC for a mechanical-only detent.
     * @param numPins    N — number of entries in pins (valid indices 0..N-1).
     * @param reverse    false (default): active pin reads LOW. true: active pin reads HIGH.
     */
    SwitchMultiPos(uint16_t controlId, const PinRef* pins, uint8_t numPins, bool reverse = false);

    void configure() override;   // configureAsInput() on each non-NC pin

protected:
    uint16_t readRaw() override; // one-hot scan → index, PIN_NC index, or NO_POSITION

private:
    const PinRef* _pins;
    uint8_t       _numPins;
    bool          _reverse;
};
```

---

## Sketch Usage

```cpp
#include <Wire.h>
#include <MCP23017.h>
#include <PanelGroup.h>
#include <Inputs/SwitchMultiPos/SwitchMultiPos.h>
#include <A4EC_CmdIds.h>   // DCSIN_* constants

MCP23017 exp1(0x20, Wire);

// Wiring map — one named PinRef per position, matching the schematic net label.
// PIN_NC marks a detent with no physical pin (e.g. a sprung OFF).
const PinRef SHRIKE_SEL_PINS[] = {
    PinRef(exp1, PORT_B, 0),   // pos 0
    PinRef(exp1, PORT_B, 1),   // pos 1
    PinRef(exp1, PORT_B, 2),   // pos 2
    PinRef(exp1, PORT_B, 3),   // pos 3
    PinRef(exp1, PORT_B, 4),   // pos 4
};
OpenSkyhawk::SwitchMultiPos shrikeSel(DCSIN_SHRIKE_SEL_KNB, SHRIKE_SEL_PINS, 5);  // 5-pos, active-LOW

void setup() {
    Wire.begin();
    PanelGroup::registerExpander(exp1, PB3, PB4);
    PanelGroup::setup();   // inits expanders, boot EVT burst (forceReport on every input)
}

void loop() {
    PanelGroup::loop();    // polls every input, drains CAN — SwitchMultiPos needs nothing else
}
```

---

## Key Data Structures

No structs beyond the private members. The position array is owned by the caller; the class
stores only a pointer (`_pins`). All per-instance state shared with the base lives in
`MultiPosInput`.

---

## Implementation Notes

### One-hot read

```cpp
uint16_t SwitchMultiPos::readRaw() {
    uint16_t ncIdx = NO_POSITION;          // nothing active → base holds the last position
    for (uint8_t i = 0; i < _numPins; i++) {
        if (_pins[i].isNC()) {
            ncIdx = i;                     // mechanical-only detent → fallback position
            continue;
        }
        bool active = _reverse ? _pins[i].read() : !_pins[i].read();
        if (active) return i;              // one-hot: first active pin wins
    }
    return ncIdx;                          // PIN_NC index, else NO_POSITION
}
```

Ported from DCS-BIOS `SwitchMultiPosT::readState` (`Switches.h`). Differences from DCS-BIOS:
- reads through `PinRef` (direct GPIO **or** MCP23017), not raw `digitalRead`;
- external pull-ups (OpenSkyhawk convention), not `INPUT_PULLUP`;
- the 20 ms debounce + emit-on-change live in the `MultiPosInput` base (DCS-BIOS emits raw on
  every change). Index semantics are identical, so DCS sees the same position numbers.

### configure()

```cpp
void SwitchMultiPos::configure() {
    for (uint8_t i = 0; i < _numPins; i++) {
        PinRef p = _pins[i];               // pin array is const (the wiring map)
        if (!p.isNC()) p.configureAsInput();
    }
}
```

The pin array is caller-owned and `const`. `PinRef::configureAsInput()` is non-const but mutates
no `PinRef` state — it only configures hardware — so a local copy of each pin configures the same
GPIO / MCP23017 channel. `PIN_NC` entries are skipped (and would be a no-op anyway). Called by
`PanelGroup::setup()` after expander `begin()`; never from the constructor.

### Debounce, hold-last, jump-safety

All provided by `MultiPosInput` — see `MultiPosInput.md`. `SwitchMultiPos` selects a 20 ms
window via the base constructor.

---

## Dependencies

| Dependency | Source | Notes |
|---|---|---|
| MultiPosInput | `Firmware/Libraries/PanelGroup/Inputs/MultiPosInput` | base: debounce, emit-on-change, EVT |
| PanelGroup | `Firmware/Libraries/PanelGroup` | InputBase, PinRef, PIN_NC |
| CANProtocol | `Firmware/Libraries/CANProtocol` | ControlPacket, sendBatched (via base) |
