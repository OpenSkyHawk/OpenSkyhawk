# Control Types

A PanelGroup sketch is mostly a list of **control objects** — one per physical switch, knob,
LED, or gauge. Inputs read hardware and fire CAN events; outputs receive DCS state and drive
hardware. This page is the catalogue, with honest Phase status: most types are **specified but
not yet implemented**.

!!! warning "Most control types are not implemented yet"
    Implemented today: **LED**, **DrumDisplay**, and **NeedleGauge** (outputs) and **Switch2Pos** +
    **SwitchMultiPos** + **AnalogMultiPos** + **RotaryEncoder** (inputs), plus the **PinRef** abstraction. LED / Switch2Pos / PinRef are
    hardware-verified (Phase 3); DrumDisplay is hardware-verified (mux + readouts on real OLEDs).
    NeedleGauge and RotaryEncoder are authored and compile-gated, with the on-hardware bench still pending; SwitchMultiPos and AnalogMultiPos are hardware-verified.
    Everything marked *Phase 4* or *Phase 5* below is specified but **not yet written** — don't expect it
    to compile today.

## PinRef — the hardware abstraction *(implemented)*

Every input and output takes a `PinRef`, not a raw pin number. One interface over three
hardware backends:

| Backend | Constructor | Notes |
|---------|-------------|-------|
| STM32 GPIO | `PinRef(pin)` | direct `digitalRead`/`Write`, `analogRead`/`Write` |
| MCP23017 | `PinRef(chip, PORT_A\|PORT_B, bit)` | digital I/O expander |
| ADS1115 | `PinRef(adc, channel)` | analog input, channels 0–3 |

Only direct STM32 GPIO can do PWM or servo output — `isGpio()` reports the backend type.

**Routing is by `controlId`, not by class.** The same input class drives a DCS-BIOS control or
a HID button depending on the `controlId` you give it:

```cpp
// DCS-BIOS route — controlId in 0x8000–0x86FF
OpenSkyhawk::Switch2Pos masterArm(DCSIN_ARM_MASTER, PinRef(PB5));

// HID route — controlId < 0x8000
OpenSkyhawk::Switch2Pos trigger(CTRL_TRIGGER, PinRef(PA6));
```

See [DCS-BIOS vs HID](../architecture/dcsbios-vs-hid.md) for which to use.

## Input classes

| Class | Status | What it is |
|-------|--------|------------|
| `Switch2Pos` | **Implemented** | Debounced 2-position switch (20 ms). value 0/1 |
| `Switch3Pos` | Phase 4 — not started | 3-position (ON-OFF-ON). value 0/1/2 |
| `SwitchMultiPos` | **Implemented** (hardware-verified) | N-pin rotary, one active. value = index |
| `AnalogMultiPos` | **Implemented** (hardware-verified) | Resistor-ladder selector on one analog pin |
| `ActionButton` | Phase 4 — not started | Momentary; fires on press only |
| `RotaryEncoder` | **Implemented** (bench pending) | Quadrature encoder. value 0=CCW, 1=CW |
| `RotaryAcceleratedEncoder` | Phase 4 — not started | Encoder with slow/fast (4-value scheme) |
| `RotarySwitch` | Phase 4 — not started | Encoder used as an N-position absolute switch |
| `AnalogInput` | Phase 4 — not started | Continuous analog, normalised to 16-bit |
| `AngleSensorInput` | Phase 4 — not started | Hall angle sensor (AS5600/MT6701) for flight axes |
| `SwitchWithCover2Pos` | Phase 4 — not started | Guarded switch (cover + switch) |

All inputs normalise analog sources to **16-bit (0–65535)** before sending. Inputs self-register
at global scope; `PanelGroup::loop()` polls them and batches events into `EVT_n` CAN frames.

!!! note "AngleSensor base class is a documented gap"
    `AngleSensorInput` wraps an `AngleSensor` abstract base (concrete `AS5600Sensor` /
    `MT6701Sensor`). The base-class TechSpec is **missing** — it's a known gap, not yet
    specified. Don't assume the API beyond `begin()` / `readAngle()`.

## Output classes

| Class | Status | What it is |
|-------|--------|------------|
| `LED` | **Implemented** | GPIO pin driven from one bit of a DCS value |
| `DrumDisplay` | **Implemented** (hardware-verified) | OLED rolling-drum readout — multi-digit gauges (speed, lat/lon, frequency, range) + optional 2-state flag. Own library; pulls U8g2 |
| `NeedleGauge` | **Implemented** (bench pending) | Pointer/needle gauge — maps a DCS value to a motor angle over a swappable driver backend (linear or calibrated curve). Supersedes `SwitecX25Output` / `AccelStepperOutput` / `ServoOutput` |
| `AnalogOutput` | Phase 5 — not started | 16-bit DCS value → PWM duty (backlighting) |
| `IntegerOutput` | Phase 5 — not started | Raw 16-bit value to a user callback |

Outputs use DCS-BIOS **output addresses** from the generated `A4EC` headers — the address
constant plus its bitmask. Example for the implemented `LED`:

```cpp
OpenSkyhawk::LED masterCaution(A_4E_C_MASTER_CAUTION, A_4E_C_MASTER_CAUTION_AM, PinRef(PB0));
```

Note the naming: `A_4E_C_<NAME>` for the address and `A_4E_C_<NAME>_AM` for the mask — **not**
the old `_A` suffix. See [DCS-BIOS Integration](dcsbios-integration.md).

!!! note "NeedleGauge drives gauge motors through a swappable backend"
    `NeedleGauge` does only the value→angle mapping. The drive lives in a reusable **motor-driver layer**
    (`Firmware/Libraries/PanelGroup/Drivers/`): a `MotorDriver` base with a `StepperMotor` backend today —
    non-blocking, driving four coils through `PinRef` (native GPIO **or** an MCP23017 expander). One
    air-core profile covers the X27.589 / VID-29 / BKA-30 family; homing is either a mechanical hard-stop
    or a digital home sensor (switch / reed / hall / opto). Drive the X27 at **5 V** through a DRV8833. A
    `ServoMotor` backend is planned (#132).

!!! note "DrumDisplay is a separate, opt-in library"
    `DrumDisplay` lives in `Firmware/Libraries/DrumDisplay/` (not PanelGroup) so the U8g2 OLED
    driver only lands on nodes that actually use a display — add `file://../../Libraries/DrumDisplay`
    to a sketch's `lib_deps` to use it. Each readout (its digit sources, geometry, and optional
    flag) is described by a `DrumReadout` defined in the sketch, like the `PinRef` wiring map.
    Many same-address OLEDs can share one bus behind a TCA9548A via the `I2cMux` helper.

## Wiring map convention

`PinRef` bit positions and mask values must be **named constants**, never inline literals.
Define a wiring-map block at the top of each sketch — one named `PinRef` per physical
connection, matching the schematic net label — so every pin appears exactly once and traces
back to the schematic.
