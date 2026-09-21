# AnalogOutput — Technical Specification

**Status:** Ready for implementation (#288) — family base (D16)
**FirmwarePlan ref:** `FirmwarePlan/05-panelgroup-api.md` (AnalogOutput family), `FirmwarePlan/00-decisions.md` (D16)
**Depends on:** `PanelGroup.md`

---

## Responsibility

Abstract base for outputs driven by **one 16-bit DCS-BIOS value**. It owns everything the members
share, so each subclass is only its sink:

- `controlId` match on each `CTRL_BCAST` packet, then `(value & mask) >> shift` — the same
  address / mask / shift triple DCS-BIOS uses (`IntegerBuffer(address, mask, shift, …)`), so packed
  fields work as well as full-word outputs.
- **Change dedup** on the decoded value: `apply()` runs only when it differs from the last one
  applied (the first matching packet always applies). DCS-BIOS re-broadcasts full state after
  `SYNC_REQ`, so without this every subclass would repeat the same work.
- Self-registration into PanelGroup's `OutputBase` list (inherited from `OutputBase`).

Does **not** know what the value means or where it goes — that is the subclass's `apply()`. Does
**not** configure pins: a subclass with hardware overrides `configure()`.

| Member | Sink | DCS-BIOS equivalent | Spec |
|---|---|---|---|
| `Dimmer` | PWM duty on a direct GPIO timer pin | `Dimmer` | `Dimmer.md` |
| `IntegerOutput` | user callback | `IntegerBuffer` | `IntegerOutput.md` |
| `ServoOutput` *(optional, later)* | servo pulse width, non-gauge uses | `ServoOutput` | — |

Needle gauges are **not** members: they use `NeedleGauge` + a `MotorDriver` (`ServoMotor`, #132),
which add calibration and smooth motion.

---

## File Layout

```
Firmware/Libraries/PanelGroup/
└── Outputs/AnalogOutput/AnalogOutput.{h,cpp}
```

No test project of its own — the base is exercised through `Firmware/Tests/Dimmer/` and
`Firmware/Tests/IntegerOutput/` (the `controlid_filter` and `dedup` scenarios live there).

---

## Public API

```cpp
// AnalogOutput.h

#pragma once
#include <PanelGroup.h>   // OutputBase

namespace OpenSkyhawk {

/**
 * @brief Abstract base for outputs driven by one 16-bit DCS-BIOS value (D16).
 *
 * Matches controlId, decodes (value & mask) >> shift, and calls apply() when the decoded value
 * changes. Subclasses implement apply() (and configure() if they own hardware).
 */
class AnalogOutput : public OutputBase {
public:
    /** @brief Match, decode, dedup, then apply(). Called by PanelGroup for every CTRL_BCAST packet. */
    void onControlPacket(uint16_t controlId, uint16_t value) final;

protected:
    /**
     * @param controlId  DCS-BIOS output address (A_4E_C_* from A4EC_OutputIds.h).
     * @param mask       bits of the 16-bit word that belong to this output (default: all).
     * @param shift      right-shift applied after masking (default 0).
     */
    AnalogOutput(uint16_t controlId, uint16_t mask = 0xFFFF, uint8_t shift = 0);

    /** @brief Sink hook — called with the decoded value when it changed. */
    virtual void apply(uint16_t value) = 0;

private:
    uint16_t _controlId;
    uint16_t _mask;
    uint8_t  _shift;
    uint16_t _lastValue;
    bool     _hasValue;   // false until the first matching packet
};

} // namespace OpenSkyhawk
```

---

## Implementation Notes

```cpp
void AnalogOutput::onControlPacket(uint16_t controlId, uint16_t value) {
    if (controlId != _controlId) return;
    const uint16_t v = (uint16_t)((value & _mask) >> _shift);
    if (_hasValue && v == _lastValue) return;   // change dedup
    _lastValue = v;
    _hasValue  = true;
    apply(v);
}
```

`onControlPacket()` is `final` so every member gets the same matching and dedup; a subclass that
needs finer dedup (e.g. `Dimmer`, where different values can map to the same duty) adds it inside
`apply()`. No `update()` override — nothing in the base needs per-loop work.

---

## Dependencies

| Dependency | Source | Notes |
|---|---|---|
| PanelGroup | `Firmware/Libraries/PanelGroup` | `OutputBase`, CTRL_BCAST dispatch |
