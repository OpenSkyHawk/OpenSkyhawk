# IntegerOutput — Technical Specification

**Status:** Ready for implementation (#288) — `AnalogOutput` family member (D16)
**FirmwarePlan ref:** `FirmwarePlan/05-panelgroup-api.md` (AnalogOutput family), `FirmwarePlan/00-decisions.md` (D16)
**Depends on:** `AnalogOutput.md`, `PanelGroup.md`

---

## Responsibility

`IntegerOutput : AnalogOutput` — the `DcsBios::IntegerBuffer` equivalent, and the escape hatch for
outputs no built-in class covers (a custom display, an LCD, a bespoke actuator). Calls a
user-supplied callback with the decoded value whenever it changes. No PinRef — the callback owns any
hardware.

Matching, `(value & mask) >> shift` decoding and change dedup come from the `AnalogOutput` base, so
the callback runs once per change, not once per `CTRL_BCAST`. It runs in `PanelGroup::loop()`
context (never from an ISR) and must not block: a slow callback delays every other output and the
next CAN drain.

---

## File Layout

```
Firmware/Libraries/PanelGroup/
└── Outputs/IntegerOutput/IntegerOutput.{h,cpp}
```

### Test project

```
Firmware/Tests/IntegerOutput/
├── platformio.ini
└── tests/
    ├── callback/          — a matching packet invokes the callback once with the value
    ├── mask_shift/        — (value & mask) >> shift is what the callback receives
    ├── controlid_filter/  — a non-matching controlId never invokes the callback
    └── dedup/             — a repeated identical value invokes the callback once
```

Logic-only scenarios: inject packets through `onControlPacket()` and read PASS/FAIL on serial — no
wiring needed. `platformio.ini` follows `Firmware/Tests/LED/`.

---

## Public API

```cpp
// IntegerOutput.h

#pragma once
#include <Outputs/AnalogOutput/AnalogOutput.h>

namespace OpenSkyhawk {

class IntegerOutput : public AnalogOutput {
public:
    using Callback = void (*)(uint16_t value);

    /**
     * @param controlId  DCS-BIOS output address (A_4E_C_* from A4EC_OutputIds.h).
     * @param callback   called with the decoded value when it changes. Must not be nullptr.
     * @param mask       bits of the 16-bit word that belong to this output (default: all).
     * @param shift      right-shift applied after masking (default 0).
     */
    IntegerOutput(uint16_t controlId, Callback callback, uint16_t mask = 0xFFFF, uint8_t shift = 0);

protected:
    void apply(uint16_t value) override { _callback(value); }

private:
    Callback _callback;
};

} // namespace OpenSkyhawk
```

---

## Sketch Usage

```cpp
#include <PanelGroup.h>
#include <Outputs/IntegerOutput/IntegerOutput.h>
#include <A4EC_OutputIds.h>

void onCanopyPos(uint16_t v) {
    // custom drive / display — keep it short, this runs in PanelGroup::loop()
}
OpenSkyhawk::IntegerOutput canopy(A_4E_C_CANOPY_POS, onCanopyPos, A_4E_C_CANOPY_POS_AM);
```

---

## Dependencies

| Dependency | Source | Notes |
|---|---|---|
| AnalogOutput | `Outputs/AnalogOutput` | family base — matching, decode, dedup |
