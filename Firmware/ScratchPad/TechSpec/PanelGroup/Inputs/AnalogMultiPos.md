# AnalogMultiPos — Technical Specification

**Status:** Not started
**FirmwarePlan ref:** `FirmwarePlan/05-panelgroup-api.md#analogmultipos-new`
**Depends on:** `PinRef.md`, `PanelGroup.md`

---

## Responsibility

Resistor-ladder multi-position selector using a single analog PinRef. Emits position index
0–(N-1). Supports ANALOG_NC (0xFFFF) sentinel, equal-division shorthand, and configurable
detection dead-band. Polls every 8ms.

---

*Spec not yet written. See FirmwarePlan refs above for behavioural requirements.*
