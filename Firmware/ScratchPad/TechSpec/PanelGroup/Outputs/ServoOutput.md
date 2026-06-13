# ServoOutput — Technical Specification

**Status:** Not started
**FirmwarePlan ref:** `FirmwarePlan/05-panelgroup-api.md#servooutput-new`
**Depends on:** `PinRef.md`, `PanelGroup.md`

---

## Responsibility

Drives an RC servo via Arduino Servo library (1–2ms pulse, 50Hz). Maps 16-bit DCS value
to pulse width via map(). Takes a `PinRef` but requires `pin.isGpio()` and uses
`pin.gpioPin()` for `Servo.attach()`. MCP23017, ADS1115, and NC pins are invalid.
Optional custom mapping function.

---

*Spec not yet written. See FirmwarePlan refs above for behavioural requirements.*
