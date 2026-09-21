# SwitchWithCover2Pos — Technical Specification

**Status:** Ready for implementation (#293, Firmware v0.1.0) — `Switch2Pos` family member (D16). The A-4E-C uses it 0× (see below).
**FirmwarePlan ref:** `FirmwarePlan/05-panelgroup-api.md`, `FirmwarePlan/00-decisions.md` (D16)
**Depends on:** `Switch2Pos.md`, `PinRef.md`, `PanelGroup.md`

---

## Responsibility

The `DcsBios::SwitchWithCover2Pos` equivalent (`src/internal/Switches.h`): **one physical switch
pin** driving **two** sim controls — the guard cover and the switch it protects. The cover is not
sensed; the class sequences it so the sim's cover animates and any sim logic gated on the cover is
satisfied:

| Physical switch goes | Frames sent (each ≥ `COVER_DELAY_MS` = 200 ms apart) | End state |
|---|---|---|
| **on** | cover `1` (open) → switch `1` | cover open, switch on |
| **off** | switch `0` → cover `0` (close) | cover closed, switch off |

States follow DCS-BIOS: `OFF_CLOSED` → `OFF_OPEN` → `ON_OPEN` and back; the 20 ms debounce is
inherited from `Switch2Pos`. On boot / `SYNC_REQ` it asserts the settled pair for the current
switch position (cover closed + switch off, or cover open + switch on), cover first when opening.

Does **not** read a cover microswitch — a builder with a sensed cover wires it as a separate
`Switch2Pos` on the cover's own control instead. Does **not** handle 3-position guarded switches:
DCS-BIOS's class is 2-position only.

### A-4E-C usage: none

The A-4E-C exposes a single cover, `AFCS_1N2_COVER`, and it guards **AFCS 1-N-2 — a 3-position
switch** (`AFCS_1N2`, spring-loaded to N). The mod does not gate that switch on the cover
(`afcs_test()` in `Cockpit/Scripts/Systems/afcs2.lua` never reads `afcs_test_guard`). So the A-4 build
is a `Switch3Pos` on `AFCS_1N2`, plus an optional `Switch2Pos` on a cover microswitch for
`AFCS_1N2_COVER` if the sim's cover should animate. This class exists for DCS-BIOS parity and for
other aircraft.

---

## Design — built on `Switch2Pos` through a protected hook (option A, PR #292 review)

`Switch2Pos` today keeps its pin/debounce state `private` and sends CAN inline in `poll()` and
`forceReport()`, so there is nothing a subclass can override. Implementing this class therefore
starts with a small, **behaviour-neutral** refactor of `Switch2Pos` — its public API, frames and
timing stay identical, and its six test envs must pass unchanged:

```cpp
class Switch2Pos : public InputBase {
    // public API unchanged
protected:
    /** Send the debounced state. Default: one ControlPacket {controlId, active} on EVT_n.
     *  poll() calls it on a confirmed change; forceReport() calls it with init = true. */
    virtual void emit(bool active, bool init);

    uint16_t _controlId;
    PinRef   _pin;
    bool     _reverse;
    bool     _lastConfirmed;
    // debounce fields stay private — subclasses only ever see confirmed states
};
```

`SwitchWithCover2Pos : Switch2Pos` adds a second id and a three-state sequencer, mirroring
`DcsBios::SwitchWithCover2Pos` (`OFF_CLOSED` → `OFF_OPEN` → `ON_OPEN` and back):

| Physical switch | Frames, each ≥ `COVER_DELAY_MS` (200 ms) after the previous | End state |
|---|---|---|
| flips **on** | cover `1` (open) → switch `1` | cover open, switch on |
| flips **off** | switch `0` → cover `0` (close) | cover closed, switch off |

- `emit()` is overridden to **set the target**, not send: the sequencer then steps one frame at a
  time from `poll()` (override calls `Switch2Pos::poll()` first, then advances), never with
  `delay()`. Flipping back mid-sequence walks the machine back the other way, as in DCS-BIOS.
- **Boot / `SYNC_REQ` (`init = true`) re-asserts the settled pair** in the same order and with the
  same spacing — cover first when on, switch first when off. This deliberately differs from
  DCS-BIOS, whose `resetState()` realigns its state machine but re-sends nothing: D5 requires every
  absolute input to be re-sent on sync, because a missed input leaves DCS wrong until the next
  physical change.
- Constructor: `(switchId, coverId, PinRef pin, bool reverse = false)`; both ids are ordinary
  `DCSIN_*` constants, sent as ABS on `EVT_n`.
- The generator already maps covers as their own `DCSIN_*` (e.g. `AFCS_1N2_COVER`), so no generator
  change is needed.

### Tests (`Firmware/Tests/SwitchWithCover2Pos`)

| Env | Asserts |
|---|---|
| `sequence_on` | on: cover 1 then switch 1, ≥ 200 ms apart |
| `sequence_off` | off: switch 0 then cover 0, ≥ 200 ms apart |
| `reverse_midway` | flipping back mid-sequence walks back without an extra frame |
| `sync_reassert` | `forceReport()` re-sends the settled pair in order |
| `switch2pos_unchanged` | the existing `Switch2Pos` envs pass with the hook in place |

```cpp
OpenSkyhawk::SwitchWithCover2Pos masterArm(DCSIN_EXAMPLE_SWITCH, DCSIN_EXAMPLE_COVER, PinRef(PB0));
```
