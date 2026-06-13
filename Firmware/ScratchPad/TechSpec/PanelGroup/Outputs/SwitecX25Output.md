# SwitecX25Output — Technical Specification

**Status:** Not started
**FirmwarePlan ref:** `FirmwarePlan/05-panelgroup-api.md#switecx25output-new`, `FirmwarePlan/08-hardware-firmware-contracts.md#drv8833pw--sleep-pin-contract`
**Depends on:** `PanelGroup.md`

---

## Responsibility

Drives X27.589 Switec stepper via DRV8833PW using the SwitecX25 library. Owns ~SLEEP pin
(HIGH from setup()), software homing via motor.reset(), and non-blocking update() per loop
iteration. Two motors on one DRV8833 share the same SLEEP_PIN.

---

*Spec not yet written. See FirmwarePlan refs above for behavioural requirements.*
