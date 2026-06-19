# MotorDriver — Technical Specification

**Status:** Done
**FirmwarePlan ref:** `FirmwarePlan/05-panelgroup-api.md#switecx25output-new` (shared base-class direction)
**Depends on:** `PanelGroup.md`
**GitHub:** #122

---

## Responsibility

Abstract base for non-blocking motor/servo drivers. A `MotorDriver` moves a physical actuator
(gauge stepper, geared stepper, RC servo) toward a commanded position without blocking the loop.
It owns the low-level drive (coil energising / PWM), homing, and per-step timing. It knows
**nothing** about DCS-BIOS.

High-level controls *compose* a `MotorDriver` and drive it: `NeedleGauge` (value→angle) today, and
later a motorised DrumDisplay or trim indicator. This is the ticket's "shared low-level motor/servo
driver base" — it exists so no stepper/servo code is duplicated across controls.

A `MotorDriver` is **not** an `OutputBase`: it is owned and ticked (`update()`) by whatever control
uses it, not registered on PanelGroup's output list.

---

## File Layout

```
Firmware/Libraries/PanelGroup/
└── Drivers/MotorDriver/MotorDriver.h     (header-only abstract base)
```

Concrete drivers live beside it: `Drivers/StepperMotor/` (now); `Drivers/ServoMotor/`,
`Drivers/StepDirMotor/` (backlog).

---

## Public API

```cpp
namespace OpenSkyhawk {

class MotorDriver {
public:
    virtual ~MotorDriver() = default;
    virtual void    configure() = 0;        // pin modes / drive hardware (after bus/board init)
    virtual void    home()      = 0;        // establish zero reference (may block at boot)
    virtual void    moveTo(int32_t pos) = 0;// set target in driver-native units (non-blocking)
    virtual void    update()    = 0;        // advance one increment toward target if due
    virtual int32_t position() const = 0;   // current position, driver-native units
};

} // namespace OpenSkyhawk
```

---

## Key Data Structures

None. Positions are `int32_t` in **driver-native units** — steps for `StepperMotor`, and
(for a future `ServoMotor`) microseconds. The owning control maps DCS-BIOS values into that space.

---

## Implementation Notes

- **Why an abstract base, not a template:** runtime polymorphism lets one `NeedleGauge` hold a
  `MotorDriver&` and accept any backend chosen at the sketch. Because every present backend drives
  through `PinRef` with no heavy per-backend library, runtime selection costs zero flash (the vtable
  is one pointer) — this resolves the ticket's compile-time-vs-runtime open question in favour of
  runtime.
- **Lifecycle:** `configure()` → `home()` once at boot (from the owner's `configure()`), then
  `moveTo()` on each new value and `update()` every loop.

---

## Dependencies

| Dependency | Source | Notes |
|---|---|---|
| PanelGroup | `Firmware/Libraries/PanelGroup` | provides `PinRef`; concrete drivers live under it |
