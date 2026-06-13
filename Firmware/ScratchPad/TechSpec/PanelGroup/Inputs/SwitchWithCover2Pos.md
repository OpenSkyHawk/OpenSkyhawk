# SwitchWithCover2Pos — Technical Specification

**Status:** Not started
**FirmwarePlan ref:** `FirmwarePlan/05-panelgroup-api.md`, `FirmwarePlan/04-dcs-bios-integration.md#generator-mapping-rules`
**Depends on:** `Switch2Pos.md`, `PinRef.md`, `PanelGroup.md`

---

## Responsibility

Paired guarded switch: a physical guard cover and the switch it protects, each with its own
`PinRef`, combined into one input object. Models the DCS-BIOS "paired boolean" pattern where
a cover and switch share a single DCS-BIOS control name but require two separate physical
inputs.

Does **not** allow the protected switch to fire events while the cover is closed. Does **not**
emit separate events for the cover position — the cover state is internal guard logic only.

---

*Spec not yet written. See FirmwarePlan refs above for behavioural requirements.*

*Note: this class is the prerequisite for generating `AFCS_1N2_COVER` (and any future cover
controls) in `A4EC_InputMap.h`. Until it is implemented, those controls remain in
`GENERATOR_GAPS.md`.*
