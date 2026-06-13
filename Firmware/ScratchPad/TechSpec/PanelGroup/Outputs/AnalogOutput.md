# AnalogOutput — Technical Specification

**Status:** Not started
**FirmwarePlan ref:** `FirmwarePlan/05-panelgroup-api.md#analogoutput-new`
**Depends on:** `PinRef.md`, `PanelGroup.md`

---

## Responsibility

Maps 16-bit DCS value to PWM duty cycle (value >> 8) on a direct STM32 GPIO pin. Used for
backlight dimming (3 zones per MCU board). Must be direct GPIO PinRef — MCP23017 cannot
do PWM. Constructor checks `pin.isGpio()`; sketches must still choose PWM-capable direct
pins from the board wiring map. Optional custom scale function.

---

*Spec not yet written. See FirmwarePlan refs above for behavioural requirements.*
