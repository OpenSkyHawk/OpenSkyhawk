# AccelStepperOutput — Technical Specification

**Status:** Not started
**FirmwarePlan ref:** `FirmwarePlan/05-panelgroup-api.md#accelstepperoutput-new`
**Depends on:** `PanelGroup.md`

---

## Responsibility

Drives a gauge stepper via AccelStepper library with a physical home sensor (hall-effect or
optical). ~SLEEP HIGH from setup(). Homing: step toward homePin until sensor active, then
currentPosition = 0. Non-blocking run() per loop iteration.

---

*Spec not yet written. See FirmwarePlan refs above for behavioural requirements.*
