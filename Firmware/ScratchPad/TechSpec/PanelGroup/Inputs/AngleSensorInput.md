# AngleSensorInput — Technical Specification

**Status:** Not started
**FirmwarePlan ref:** `FirmwarePlan/05-panelgroup-api.md#anglesensorinput-new`
**Depends on:** `PinRef.md`, `PanelGroup.md`

---

## Responsibility

Hall-effect angle sensor for flight control axes (AS5600 / MT6701). Applies calibration
(centerDeg, travelDeg), dead-band (32 counts), and EWMA filtering. Polls every 8ms.
Always routes via HID (controlId < 0x8000). Output 0–65535 converted to ±32767 in
SimGateway HIDAxis lambda. Wrap-around constraint applies to calibration range.

---

*Spec not yet written. See FirmwarePlan refs above for behavioural requirements.*
