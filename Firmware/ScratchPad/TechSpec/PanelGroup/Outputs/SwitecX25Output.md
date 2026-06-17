# SwitecX25Output — Technical Specification

**Status:** Not started
**FirmwarePlan ref:** `FirmwarePlan/05-panelgroup-api.md#switecx25output-new`, `FirmwarePlan/08-hardware-firmware-contracts.md#drv8833pw--sleep-pin-contract`
**Depends on:** `PanelGroup.md`

---

## Responsibility

Drives X27.589 Switec stepper via DRV8833PW using the SwitecX25 library. Owns ~SLEEP pin
(HIGH from setup()), software homing via motor.reset(), and non-blocking update() per loop
iteration. One DRV8833 drives one bipolar stepper (2 coils); multiple drivers may share one
SLEEP_PIN to save GPIO.

---

*Spec not yet written. See FirmwarePlan refs above for behavioural requirements.*
