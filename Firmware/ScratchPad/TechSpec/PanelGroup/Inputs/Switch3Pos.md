# Switch3Pos — Technical Specification

**Status:** Done (hardware-verified — 6/6 envs PASS 2026-06-23)
**FirmwarePlan ref:** `FirmwarePlan/05-panelgroup-api.md#switch3pos-new`
**Depends on:** `MultiPosInput.md`, `PinRef.md`, `PanelGroup.md`

---

## Responsibility

Three-position switch (ON-OFF-ON / spring-centred) on two digital `PinRef`s. Emits the position
index **0 / 1 / 2** over CAN (`MULTIPOS` dispatch). A subclass of `MultiPosInput`: it provides only
the **read**; the base owns the debounce, emit-on-change, hold-last, and EVT transmission.

Handles:
- **two-pin read** — pin A active → 0, pin B active → 2, neither active → 1 (**centre**);
- **centre as a real position** — resolved directly (the OFF/centre detent has no pin of its own),
  so `readRaw()` never returns `NO_POSITION` and the base's hold-last path is unused;
- **both-active** (mechanically impossible — a bounce during a throw) → pin A wins (0), matching
  DcsBios; the debounce absorbs the transient either way;
- **active level** — `reverse = false` (default): active pin reads LOW; `reverse = true`: active
  reads HIGH.

Does **not** interpret `controlId`, debounce on its own (the base does), or enable internal
pull-ups (board wiring supplies bias).

---

## File Layout

```
Firmware/Libraries/PanelGroup/Inputs/Switch3Pos/
├── Switch3Pos.h
└── Switch3Pos.cpp
```

### Test project — `Firmware/Tests/Switch3Pos/`

GPIO-driven bench tests (mirror `Tests/SwitchMultiPos/`): two control output pins drive the two
switch input pins through jumpers; `check(label, ok)` → `STM32Board::diagSerial()` PASS/FAIL. The
resolved index is asserted directly via `Switch3Pos::position()` and the emit count via the
`#ifdef MULTIPOS_TEST` `emitCount()` seam — **not** a captured CAN frame, so there is no loopback
fragility. CAN runs in **normal mode** (`CANProtocol::start()`): the node ACKs the (unmodified)
PanelBridge. Rig: jumper `PB0→PA0` (pin A), `PB1→PA1` (pin B).

| Scenario env | Verifies |
|---|---|
| `test_positions` | pin A → 0, neither → 1 (centre), pin B → 2; three forceReports → 3 EVTs |
| `test_both_active` | both pins active → 0 (pin A priority) |
| `test_debounce` | a change must hold 20 ms before EVT; sub-window bounce emits nothing |
| `test_no_duplicate` | a held position emits exactly once |
| `test_reverse` | `reverse = true` flips the active level (active = HIGH) |
| `test_force_report` | boot read emits the current index once; repeats on a second call |

---

## Public API

```cpp
class Switch3Pos : public MultiPosInput {
public:
    static constexpr uint16_t DEBOUNCE_MS = 20;

    /**
     * @param controlId   DCSIN_* or CTRL_* constant. Determines PanelBridge routing.
     * @param pinA        outer throw → position 0 (active-LOW unless reverse).
     * @param pinB        outer throw → position 2.
     * @param reverse     false (default): active pin reads LOW. true: active pin reads HIGH.
     * @param debounceMs  index stability window before a change is confirmed (default 20 ms).
     */
    Switch3Pos(uint16_t controlId, PinRef pinA, PinRef pinB, bool reverse = false,
               uint16_t debounceMs = DEBOUNCE_MS);

    void configure() override;   // configureAsInput() on both pins

protected:
    uint16_t readRaw() override; // pinA→0, pinB→2, neither→1 (pinA priority if both active)

private:
    PinRef _pinA;
    PinRef _pinB;
    bool   _reverse;
};
```

---

## Sketch Usage

```cpp
#include <Wire.h>
#include <MCP23017.h>
#include <PanelGroup.h>
#include <Inputs/Switch3Pos/Switch3Pos.h>
#include <A4EC_CmdIds.h>   // DCSIN_* constants

MCP23017 exp1(0x20, Wire);

// AN/ASN-41 destination-latitude slew: spring-centred L / centre / R on two pins.
OpenSkyhawk::Switch3Pos latSlew(DCSIN_ASN41_LAT_SLEW,
                                PinRef(exp1, PORT_A, 0),    // pin A → position 0 (e.g. L)
                                PinRef(exp1, PORT_A, 1));   // pin B → position 2 (e.g. R)

void setup() {
    Wire.begin();
    PanelGroup::registerExpander(exp1, PB12, PB13);
    PanelGroup::setup();   // inits expanders, boot EVT burst (forceReport on every input)
}

void loop() {
    PanelGroup::loop();    // polls every input, drains CAN — Switch3Pos needs nothing else
}
```

---

## Key Data Structures

None beyond the two `PinRef` members (`_pinA`, `_pinB`) and `_reverse`. All shared per-instance
state lives in `MultiPosInput`. Unlike `SwitchMultiPos`, there is no caller-owned array — the two
pins are stored by value.

---

## Implementation Notes

### Two-pin read

```cpp
uint16_t Switch3Pos::readRaw() {
    bool a = _reverse ? _pinA.read() : !_pinA.read();   // active
    bool b = _reverse ? _pinB.read() : !_pinB.read();
    if (a) return 0;   // pin A priority — matches DcsBios both-active behaviour
    if (b) return 2;
    return 1;          // centre — a real position, never NO_POSITION
}
```

Ported from DcsBios `Switch3PosT::readState` (`Switches.h`: `pinA LOW → 0; pinB LOW → 2; else 1`).
Differences from DcsBios:
- reads through `PinRef` (direct GPIO **or** MCP23017), not raw `digitalRead`;
- external pull-ups (OpenSkyhawk convention), not `INPUT_PULLUP`;
- the 20 ms debounce + emit-on-change live in the `MultiPosInput` base (DcsBios emits raw on every
  change). Index semantics are identical, so DCS sees the same position numbers.

**FirmwarePlan reconciliation:** the earlier stub specified "both pins active → retain last valid
state." This implementation instead matches **DcsBios (pin A priority → 0)** — both-active is a
mechanically impossible transient that the 20 ms debounce absorbs regardless, and matching DcsBios
keeps the wire behaviour identical. `FirmwarePlan/05` is updated to this rule in this PR.

### configure()

```cpp
void Switch3Pos::configure() {
    _pinA.configureAsInput();
    _pinB.configureAsInput();
}
```

Does not enable internal pull-ups; board wiring supplies bias. Called by `PanelGroup::setup()`
after expander `begin()`; never from the constructor (MCP23017 register writes need the expander
initialised first).

### Debounce, emit-on-change, forceReport

All provided by `MultiPosInput` — see `MultiPosInput.md`. `Switch3Pos` selects a 20 ms window via
the base constructor (`MultiPosInput(controlId, 3, debounceMs)`).

---

## Dependencies

| Dependency | Source | Notes |
|---|---|---|
| MultiPosInput | `Firmware/Libraries/PanelGroup/Inputs/MultiPosInput` | base: debounce, emit-on-change, EVT |
| PanelGroup | `Firmware/Libraries/PanelGroup` | InputBase, PinRef |
| CANProtocol | `Firmware/Libraries/CANProtocol` | ControlPacket, sendBatched (via base) |
