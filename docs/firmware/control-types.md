# Control Types

A PanelGroup sketch is mostly a list of **control objects** — one per physical switch, knob,
LED, or gauge. Inputs read hardware and fire CAN events; outputs receive DCS state and drive
hardware. This page is the catalogue, with honest status: every input the A-4E-C needs is
implemented; one output family is still to come before the first release.

!!! warning "What's built and what isn't"
    Implemented today: **Switch2Pos**, **Switch3Pos**, **SwitchMultiPos**, **AnalogMultiPos**,
    **AnalogInput**, **RotaryEncoder** and **ActionButton** (inputs); **LED**, **DrumDisplay** and
    **NeedleGauge** (outputs); plus the **PinRef** abstraction. All are hardware-verified except
    ActionButton, whose hardware and live-DCS run is part of the v1.0 soak test.
    Rows marked *Planned* below are specified but **not written yet** — don't expect them to
    compile today.

## PinRef — the hardware abstraction *(implemented)*

Every input and output takes a `PinRef`, not a raw pin number (one hardware-bound exception, `Dimmer`,
is noted below). One interface over three
hardware backends:

| Backend | Constructor | Notes |
|---------|-------------|-------|
| STM32 GPIO | `PinRef(pin)` | direct `digitalRead`/`Write`, `analogRead`/`Write` |
| MCP23017 | `PinRef(chip, PORT_A\|PORT_B, bit)` | digital I/O expander |
| ADS1115 | `PinRef(adc, channel)` | analog input, channels 0–3 |

Only direct STM32 GPIO can do PWM or servo output — `isGpio()` reports the backend type. That's
why the planned `Dimmer` takes a native timer pin (`PA6`) instead of a `PinRef`: PWM comes from an
STM32 timer, so an expander pin is a build error rather than a silent no-op.

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
| `Switch3Pos` | **Implemented** (hardware-verified) | 3-position (ON-OFF-ON). value 0/1/2 |
| `SwitchMultiPos` | **Implemented** (hardware-verified) | N-pin rotary, one active. value = index |
| `AnalogMultiPos` | **Implemented** (hardware-verified) | Resistor-ladder selector on one analog pin |
| `ActionButton` | **Implemented** (#116) | Momentary button driving a control that latches *in the sim* — one `TOGGLE` per press, nothing on release. See the note below |
| `RotaryEncoder` | **Implemented** (dual-mode REL/DIR, #147) | Quadrature encoder, relative. REL → ±step (continuous knobs); DIR → ±1 (no-indicator selectors) |
| `RotaryAcceleratedEncoder` | Planned — v0.1.0 (#287) | `RotaryEncoder` subclass: DCS-BIOS's momentum filter for noisy encoders, plus a bigger step when spun fast |
| `RotarySwitch` | Not planned | Use `RotaryEncoder` in DIR mode — see the note below |
| `AnalogInput` | **Implemented** (hardware-verified) | Continuous analog, normalised to 16-bit (EWMA + hysteresis) |
| `AngleSensorInput` | Planned — v0.1.0 (#294) | `AnalogInput` subclass reading a magnetic angle sensor (AS5600/MT6701) — absolute knobs such as gunsight elevation |
| `SwitchWithCover2Pos` | Planned — v0.1.0 (#293) | One switch pin driving a guarded sim control: opens the sim's cover before the switch and closes it after (DCS-BIOS behaviour) |

All inputs normalise analog sources to **16-bit (0–65535)** before sending. Inputs self-register
at global scope; `PanelGroup::loop()` polls them and batches events into CAN frames — `EVT_n`
for an absolute value, `EVT_REL_n` and `EVT_DIR_n` for `RotaryEncoder`'s two modes, and
`EVT_ACTION_n` for `ActionButton`. See [CAN Bus](../architecture/can-bus.md) for the frame IDs
and why the event families are split.

!!! note "ActionButton lets a momentary button drive a switch that latches in the sim"
    `Switch2Pos` sends the switch's absolute position, so it needs a physical part that latches.
    `ActionButton` instead sends one `TOGGLE` per press: DCS-BIOS reads the control's current
    value, flips it, and writes it back. Each press toggles and the state persists — so a plain
    momentary button can drive a two-position cockpit switch.

    A second benefit falls out of that. Because the sim supplies the current value, the panel
    **cannot desync**. A physical latching switch can end up disagreeing with the sim after a cold
    start, a keyboard binding, or a mission script, and then it physically lies about the state
    until you cycle it. A momentary button has no position to be wrong about.

    The trade-off is you lose at-a-glance state — the panel can't show you what's on without a
    separate indicator. OpenSkyhawk's own panels use real latching switches for that reason;
    `ActionButton` is here for builders who'd rather not source them.

    **Only for controls DCS-BIOS declares with `defineToggleSwitch`** — ones that latch in the sim.
    Pointing it at a `definePushButton` control is wrong: the sim tracks those by physical
    position, so a `TOGGLE` latches them *on* until the next press. A lamp-test button wired this
    way would stay lit. Use `Switch2Pos` for those.

!!! note "Classes come in families, named after DCS-BIOS"
    A class that only changes where an existing one reads from, or what it drives, is a subclass
    of it — `RotaryAcceleratedEncoder` extends `RotaryEncoder`, `AngleSensorInput` extends
    `AnalogInput`, `Dimmer` and `IntegerOutput` extend `AnalogOutput`. Names follow
    `dcs-bios-arduino-library` wherever it has an equivalent, so a DCS-BIOS sketch translates
    class for class.

    The one deliberate gap is `RotarySwitch`. The DCS-BIOS version assumes position 0 at power-up
    and sends absolute positions, so the first click after a cold start can jump the sim away from
    a mission preset. `RotaryEncoder` in DIR mode sends "one up / one down" instead: the sim keeps
    the position and stops at the ends, and the cockpit readout shows the result.

## Output classes

| Class | Status | What it is |
|-------|--------|------------|
| `LED` | **Implemented** | GPIO pin driven from one bit of a DCS value |
| `DrumDisplay` | **Implemented** (hardware-verified) | OLED rolling-drum readout — multi-digit gauges (speed, lat/lon, frequency, range) + optional 2-state flag. Own library; pulls U8g2 |
| `NeedleGauge` | **Implemented** (hardware-verified against live DCS, #137) | Pointer/needle gauge — maps a DCS value to a motor angle over a swappable driver backend (linear or calibrated curve). Supersedes `SwitecX25Output` / `AccelStepperOutput` / `ServoOutput` |
| `AnalogOutput` | Planned — v0.1.0 (#288) | Family base for outputs driven by one DCS value: matching, decoding and change detection |
| `Dimmer` | Planned — v0.1.0 (#288) | PWM duty on a GPIO timer pin — backlight zones driven by the sim's `LIGHTS_*` intensities |
| `IntegerOutput` | Planned — v0.1.0 (#288) | The decoded value to your own callback, for custom displays (DCS-BIOS `IntegerBuffer`) |

Outputs use DCS-BIOS **output addresses** from the generated `A4EC` headers — the address
constant plus its bitmask. Example for the implemented `LED`:

```cpp
OpenSkyhawk::LED gearLight(A_4E_C_GEAR_LIGHT, A_4E_C_GEAR_LIGHT_AM, PinRef(PB0));
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
