# SwitchWithCover2Pos — Technical Specification

**Status:** Ready for implementation — scheduled after v1.0 (D16). The A-4E-C uses it 0× (see below).
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

## Design notes

- Builds on `Switch2Pos` (same pin read, polarity and debounce); overrides the emit path to run the
  two-control sequence. Two `DCSIN_*` ids at construction: `(switchId, coverId, pin, reverse)`.
- The 200 ms spacing is non-blocking — a pending second frame is sent from `poll()` once the delay
  has elapsed, never with `delay()`.
- The generator already maps `AFCS_1N2_COVER` as its own `DCSIN_*` (the old "paired boolean" gap is
  gone), so no generator change is needed.

```cpp
OpenSkyhawk::SwitchWithCover2Pos gunArm(DCSIN_EXAMPLE_SWITCH, DCSIN_EXAMPLE_COVER, PinRef(PB0));
```
