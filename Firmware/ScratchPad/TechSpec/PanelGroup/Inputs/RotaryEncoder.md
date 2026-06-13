# RotaryEncoder — Technical Specification

**Status:** Not started
**FirmwarePlan ref:** `FirmwarePlan/05-panelgroup-api.md#rotaryencoder-new`
**Depends on:** `PinRef.md`, `PanelGroup.md`

---

## Responsibility

Quadrature encoder (A/B PinRefs). Accumulates delta; emits CAN EVT (0=CCW, 1=CW) when
|delta| >= stepsPerDetent. Base class for RotaryAcceleratedEncoder and RotarySwitch.
MCP23017 interrupt latency constraint: ≤ one detent period.

---

*Spec not yet written. See FirmwarePlan refs above for behavioural requirements.*
