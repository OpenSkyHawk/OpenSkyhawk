# NeedleGauge — Technical Specification

**Status:** Not started
**FirmwarePlan ref:** `FirmwarePlan/05-panelgroup-api.md#switecx25output-new`, `FirmwarePlan/08-hardware-firmware-contracts.md#drv8833pw--sleep-pin-contract`
**Depends on:** `PanelGroup.md`, `PinRef.md`
**GitHub:** #122

---

## Responsibility

Drives a **needle / pointer gauge** from one DCS-BIOS address, mapping the 16-bit value to a
needle angle. The **motor driver backend is selected by the constructor**, so a single class
covers every pointer gauge in the aircraft (119 in the control inventory). Non-blocking
`update()` per loop iteration; owns the backend's `~SLEEP`/enable pin where required
(shareable across instances).

**Supersedes** the former `ServoOutput` and `AccelStepperOutput` specs — they are now backends.

## Backends (constructor-selected)

- **Switec X25** — X27.589 air-core stepper via DRV8833 + SwitecX25 lib; software home via
  `motor.reset()` against the internal stop. Most A-4E needle gauges.
- **AccelStepper** — gauge stepper via AccelStepper lib; physical home sensor (hall/optical),
  step toward `homePin` until active then `currentPosition = 0`.
- **Servo** — RC servo via Arduino Servo lib (1–2 ms pulse @ 50 Hz); absolute, no homing.
  Requires a GPIO `PinRef` (`pin.isGpio()`); MCP23017/ADS1115/NC invalid.

## Shared base-class architecture

`NeedleGauge` and `DrumDisplay` (#113) both drive motors. Intended layering: a **low-level
motor/servo driver base** (coil/PWM drive, `~SLEEP`/enable, position tracking, non-blocking
`update()`), with `NeedleGauge` (value→angle) and `DrumDisplay` (digits→rotation + carry) as
high-level abstractions on top. No duplicated stepper-drive code between the two.

---

*Spec not yet written. See FirmwarePlan refs above + #122 for behavioural requirements.*
