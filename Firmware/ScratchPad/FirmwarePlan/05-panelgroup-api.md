# 05 — PanelGroup API

**Owns:** `PinRef` abstraction, MCP23017 management, all input class specs, all output class specs.
**Does not own:** CAN frame IDs (→ 02), DCS-BIOS routing (→ 04), boot sequence (→ 09),
hardware pin assignments (→ 08).

> **Naming convention:** "Input" = input *to DCS* (human operates hardware → DCS receives command).
> "Output" = output *from DCS* (DCS state → hardware reflects it).

---

## PinRef — Hardware Abstraction

All input and output objects take a `PinRef` instead of a raw `uint8_t` pin.
`PinRef` abstracts three hardware types behind a single interface:

| Source | Constructor | Notes |
|--------|-------------|-------|
| Direct STM32 GPIO | `PinRef(uint8_t pin)` | `digitalRead`/`digitalWrite`/`analogRead`/`analogWrite` directly |
| MCP23017 GPIO | `PinRef(MCP23017& chip, uint8_t port, uint8_t bit)` | port = `PORT_A` or `PORT_B`; bit = 0–7 |
| ADS1115 channel | `PinRef(ADS1115& adc, uint8_t channel)` | channel = 0–3; read via `readSingleEnded()` |

**Interface:**
- `bool read()` — digital read
- `uint16_t readAnalog()` — ADC read
- `void write(bool)` — digital write
- `void writeAnalog(uint16_t)` — PWM/analog write (STM32 GPIO only)
- `bool isGpio()` — true only for direct STM32 GPIO
- `uint8_t gpioPin()` — raw Arduino pin for direct-only APIs such as `Servo.attach()`

`isGpio()` is a backend-type check, not dynamic hardware discovery. MCP23017 and ADS1115
PinRefs are never PWM/servo-capable. For direct GPIO, sketches still choose PWM/servo-capable
pins from the board wiring map.

---

## MCP23017 Management

### Sketch Responsibilities

The sketch creates and owns each MCP23017 instance, then registers each one with PanelGroup:

```cpp
MCP23017 exp1(0x20, Wire);
MCP23017 exp2(0x21, Wire);
MCP23017 exp3(0x22, Wire);

// Dedicated interrupt pins (one chip per STM32 pin):
PanelGroup::registerExpander(exp1, PB3,  PB4);   // INTA→PB3, INTB→PB4

// MIRROR mode (same pin for INTA and INTB — PanelGroup sets IOCON.MIRROR):
PanelGroup::registerExpander(exp1, PB3, PB3);    // both ports → PB3

// Shared interrupt line (wired-OR on PCB):
PanelGroup::registerExpander(exp2, PA4, PA5);
PanelGroup::registerExpander(exp3, PA4, PA5);    // exp3 also wired to the same pins
```

**Interrupt pin constraints:** Do not use PB14/PB15 — STM32Board reserves them for the
bi-color status LED. Do not use PC13/PC14/PC15 for panel I/O; custom OpenSkyhawk STM32 boards
treat them as unavailable because of RTC/tamper/32 kHz oscillator/current-limit quirks.

### Two I2C Buses

| Object | Hardware | Default pins |
|--------|----------|-------------|
| `Wire` | I2C1 | PB6 (SCL) / PB7 (SDA) |
| `Wire1` | I2C2 | PB10 (SCL) / PB11 (SDA) |

The sketch calls `Wire.begin()` / `Wire1.begin()` in `setup()` **before** `PanelGroup::setup()`.
PanelGroup never calls `begin()` on any bus. Only start buses actually in use.

### Interrupt Topologies

**A — Dedicated pins (one chip per STM32 pin):**
Each chip's INTA and INTB connect to their own STM32 GPIO. When the ISR fires, PanelGroup
knows exactly which chip triggered and reads only that chip's INTCAP. Uses 2 STM32 pins/chip.

**B — Shared interrupt line (wired-OR, multiple chips per pin):**
Two or more chips have INTA (or INTB) wire-OR'd to one STM32 GPIO. When that pin fires,
PanelGroup scans all chips on that line, checks each chip's `INTFLAG`, reads `INTCAP` for
chips with non-zero flags. Practical limit: 1–2 shared lines covering 2–4 chips each.

**MIRROR mode (intaPin == intbPin):**
If the sketch passes the same pin for both args, PanelGroup sets `IOCON.MIRROR = 1`. Either
port change fires the single shared interrupt line. Both `INTFLAG_A` and `INTFLAG_B` are read.

### Electrical Requirements for Wired-OR Lines

- `IOCON.ODR = 1` (open-drain interrupt output) — PanelGroup sets this automatically for any
  chip registered on a shared pin.
- `IOCON.INTPOL = 0` (active-LOW) — MCP23017 default.
- STM32 GPIO: `INPUT_PULLUP` (internal ~40 kΩ; external 10 kΩ recommended if > 4 chips share
  the line).
- Each chip's INT pin connects via 100 Ω series resistor (specified in `hardware-standards.md`
  MCP23017 circuit block).

### Interrupt Dispatch Sequence

At boot: full port read of every registered chip to establish baseline.

During operation:
1. Any chip interrupt fires → ISR sets volatile flag keyed to the STM32 interrupt pin. ISR is
   minimal — no I2C.
2. `PanelGroup::loop()` checks all registered interrupt pins each iteration.
3. For each flagged pin: iterate all chips on that pin, read `INTFLAG_A`/`INTFLAG_B`. For each
   chip with non-zero flag, read `INTCAP` to get captured port state at moment of interrupt.
4. Compare captured state to last-known; dispatch changed pins to all `InputBase` objects
   referencing that chip+port+bit.

### Polling Fallback

For boards with no interrupt pins wired (e.g. initial bringup), `PanelGroup::loop()` polls all
chips' `INTF` registers at ~20 ms intervals. Automatic — if no interrupt pin is registered for
a chip, it is polled.

---

## Input Classes

All input objects are declared at global scope. Constructors self-register into a static linked
list. `PanelGroup::loop()` polls all registered objects each iteration.

**Routing is determined by the `controlId` passed to the constructor — not by the class.**
The same input classes are used for both DCS-BIOS controls and HID buttons:

| `controlId` | Routed by | Destination |
|-------------|-----------|-------------|
| `DCSIN_*` (`0x8000`-`0x86FF`) | PanelBridge | `sendDcsBiosMessage()` → DCS |
| `CTRL_*` (< 0x8000) | SimGateway | `HIDButton` / `HIDAxis` → Joystick |

Example — same class, different routing:

```cpp
// DCS-BIOS route: master arm switch → sendDcsBiosMessage("ARM_MASTER", "0"/"1")
OpenSkyhawk::Switch2Pos masterArm(DCSIN_ARM_MASTER, PIN_MASTER_ARM);

// HID route: trigger button → Joystick.button(0, ...)
OpenSkyhawk::Switch2Pos trigger(CTRL_TRIGGER, PIN_TRIGGER);
```

PanelGroup emits the same `ControlPacket` either way, batched into an `EVT_n`
`ControlPacketPair` on CAN — the `controlId` value is what causes PanelBridge to route it
differently. PanelGroup submits individual input packets to `CANProtocol::sendBatched()`;
CANProtocol owns the `ControlPacketPair` builder and queue. If slot A is queued and no second
input event arrives, CANProtocol must flush the half-full EVT batch within two
`PanelGroup::loop()` iterations using slot B `controlId = 0x0000`. During boot and `SYNC_REQ`
full-state polls, PanelGroup calls `CANProtocol::flushBatched(canIdEvt(NODE_ID))` so the odd
trailing packet flushes immediately at the end of the poll pass.

### Switch2Pos *(exists — needs PinRef update)*

Debounced 2-position switch. VALUE: 1 = active (pin LOW), 0 = inactive. Debounce: 20 ms fixed.

```cpp
OpenSkyhawk::Switch2Pos masterArm(DCSIN_ARM_MASTER, PinRef(PB5));
OpenSkyhawk::Switch2Pos ejSafe   (DCSIN_SEAT_EJECT_SAFE, PinRef(expander1, PORT_A, 3));
```

### Switch3Pos *(new)*

3-position switch (ON-OFF-ON or ON-ON). VALUE: 0 = pin A active, 1 = neither (centre),
2 = pin B active. Debounce: 20 ms per state.

If both pins read active simultaneously (hardware fault or bounce during throw), reading is
invalid and last valid state is retained.

```cpp
OpenSkyhawk::Switch3Pos fuelSelector(DCSIN_FUEL_SEL,
                                      PinRef(expander1, PORT_A, 0),
                                      PinRef(expander1, PORT_A, 1));
```

### SwitchMultiPos *(new)*

Multi-position rotary selector switch. N discrete pins — exactly one active (LOW) at a time.
VALUE: position index 0 to N-1.

If no pin reads active, last valid state is retained.

Supports `PIN_NC` sentinel for positions with no physical pin (mechanical-only detents).

```cpp
const PinRef weaponPins[] = { PinRef(expander1, PORT_B, 0), PinRef(expander1, PORT_B, 1),
                               PinRef(expander1, PORT_B, 2), PinRef(expander1, PORT_B, 3) };
OpenSkyhawk::SwitchMultiPos weaponSel(DCSIN_WEAPON_SEL, weaponPins, 4);
```

### AnalogMultiPos *(new)*

Resistor-ladder multi-position selector. Single analog `PinRef` reads a different voltage per
position. VALUE: position index 0 to N-1 (same as `SwitchMultiPos`). PanelBridge dispatches
both via `MULTIPOS` — no SimGateway declarations needed.

`ANALOG_NC = 0xFFFF` (65535) marks positions with no physical detent. Physically unreachable:
STM32 ADC tops at 65520 (`analogReadResolution(16)`, framework scales 12-bit → 16-bit);
ADS1115 tops at 65534 (15-bit single-ended × 2; GAIN_ONE ±4.096V FSR, 3.3V → ~52800).

```cpp
// 5-position selector. Position 2 has no detent:
const uint16_t posVals[] = { 4000, 16000, ANALOG_NC, 42000, 56000 };
OpenSkyhawk::AnalogMultiPos modeKnob(DCSIN_MODE_SEL, PinRef(adc, 1), 5, posVals);

// Equal-spacing shorthand (all positions valid):
OpenSkyhawk::AnalogMultiPos modeKnob(DCSIN_MODE_SEL, PinRef(PA1), 5);
```

Detection bands: half the distance to each neighbour (ignoring `ANALOG_NC` entries), minus a
configurable dead-band (default 1000 counts). Polling rate: 8 ms. EVT sent only when resolved
position changes.

### ActionButton *(new)*

Momentary push button. Sends one DCS-BIOS argument on press; nothing on release. VALUE: `1`
(press only). Debounce: 20 ms fixed.

```cpp
OpenSkyhawk::ActionButton iffInc(DCSIN_IFF_CODE_INC, PinRef(exp1, PORT_A, 5));
OpenSkyhawk::ActionButton iffDec(DCSIN_IFF_CODE_DEC, PinRef(exp1, PORT_A, 6));
```

### RotaryEncoder *(new)*

Quadrature encoder (A/B pins). Mirrors `DcsBios::RotaryEncoder` behaviour: accumulates delta;
fires CAN EVT when `|delta| >= stepsPerDetent`. VALUE: 1 = clockwise, 0 = counter-clockwise.

`StepsPerDetent` enum: `ONE_STEP_PER_DETENT`, `TWO_STEPS_PER_DETENT`, `FOUR_STEPS_PER_DETENT`,
`EIGHT_STEPS_PER_DETENT`.

Interrupt latency constraint: ≤ one encoder detent period (typically ≥ 10 ms at human speeds —
achievable with ~1 ms interrupt response through MCP23017).

```cpp
OpenSkyhawk::RotaryEncoder altSet(DCSIN_ALT_SET,
                                   PinRef(expander2, PORT_A, 6),
                                   PinRef(expander2, PORT_A, 7),
                                   ONE_STEP_PER_DETENT);
```

### RotaryAcceleratedEncoder *(new)*

Accelerated variant. Tracks time between detents; encodes direction and speed into value.

PanelGroup applies the 175 ms inter-detent threshold:

| VALUE | Meaning |
|-------|---------|
| 0 | Slow CCW (≥ 175 ms since last detent) |
| 1 | Slow CW |
| 2 | Fast CCW (< 175 ms since last detent) |
| 3 | Fast CW |

Momentum tracking prevents false direction changes: a reversal while momentum is non-zero
is ignored (consumed as a braking step), matching DCS-BIOS behaviour.

PanelBridge dispatches: 0 → `arg0`, 1 → `arg1`, 2 → `arg0fast`, 3 → `arg1fast`.

```cpp
OpenSkyhawk::RotaryAcceleratedEncoder navKnob(DCSIN_PPOS_LAT_KNB,
                                               PinRef(expander1, PORT_B, 0),
                                               PinRef(expander1, PORT_B, 1),
                                               ONE_STEP_PER_DETENT);
```

### RotarySwitch *(new)*

Rotary encoder used as an N-position absolute switch. Tracks current position (0 to N-1) via
quadrature A/B encoder. Turning past either end stop (0 or N-1) is ignored — no wrap.

VALUE: current position index 0–(N-1). Dispatched as `MULTIPOS` by PanelBridge.

```cpp
OpenSkyhawk::RotarySwitch navMode(DCSIN_NAV_MODE,
                                   PinRef(exp1, PORT_B, 0),
                                   PinRef(exp1, PORT_B, 1), 5);
```

**Known gap — boot position:** Without a fiducial or index pin, `RotarySwitch` cannot determine
absolute position at power-on and initialises to 0. Physical and software positions
re-synchronise when the user turns to either end-stop. See `11-open-issues.md`.

### AnalogInput *(new)*

Continuous or stepped analog input. All sources normalised to **16-bit (0–65535)**:

| Source | Raw resolution | Conversion | Notes |
|--------|---------------|------------|-------|
| STM32 ADC | 12-bit (0–4095) | `analogReadResolution(16)` — framework scales → 0–65520 | Set in `STM32Board::begin()` |
| ADS1115 | 15-bit single-ended (0–32767) | ×2 → 16-bit (0–65534) | GAIN_ONE (±4.096V FSR); 3.3V → ~52800 |

Configurable `[minRaw, maxRaw]` input range; values outside clamped to 0 or 65535.
Polling rate: every 8 ms. Dead-band: 32 counts (configurable).

```cpp
OpenSkyhawk::AnalogInput throttle(CTRL_THROTTLE, PinRef(PA0));

ADS1115 adc(0x48, Wire);
OpenSkyhawk::AnalogInput rudder(CTRL_RUDDER, PinRef(adc, 0));
```

The 16-bit value is used as-is by both routing paths: PanelBridge passes it to
`sendDcsBiosMessage()` for DCS-BIOS controls (`MULTIPOS` type); SimGateway passes it to the
`HIDAxis` lambda for joystick axes — no rescaling at either destination.

### AngleSensorInput *(new)*

Hall-effect angle sensor for continuous-rotation flight control axes. Used on flight-control
sub-nodes, not on cockpit panel boards.

Two sensors supported via a common `AngleSensor` abstraction:

| Chip | Raw resolution | I²C address | Conversion |
|------|---------------|-------------|------------|
| AS5600 | 12-bit (0–4095) | Fixed 0x36 | ×16 → 16-bit (0–65520) |
| MT6701 | 14-bit (0–16383) | Fixed 0x06 (I²C mode) | ×4 → 16-bit (0–65532) |

Both have fixed I²C addresses. Two axes on one sub-node require two I²C buses (`Wire` /
`Wire1`).

```cpp
class AngleSensor {
public:
    virtual bool     begin()     = 0;  // init chip, returns false if not found
    virtual uint16_t readAngle() = 0;  // 0–65535 (16-bit, full 360° mapped linearly)
};

class AS5600Sensor : public AngleSensor { AS5600Sensor(TwoWire& wire); ... };
class MT6701Sensor : public AngleSensor { MT6701Sensor(TwoWire& wire); ... };
```

Calibration parameters: `centerDeg` (0–360, physical angle at neutral) and `travelDeg`
(± degrees from center mapped to 0–65535).

Dead-band: 32 counts (configurable). AS5600 increments in steps of 16 after ×16 scaling —
a 16-count dead-band suppresses nothing; 32 rejects single-step jitter. EWMA filtering applied
before sending. Polling rate: every 8 ms.

**Calibration constraint:** `[center − travel, center + travel]` must not straddle the 0°/360°
wrap boundary. If neutral is near 0° or 360°, rotate the magnet mount.

**Routing:** `AngleSensorInput` for flight control axes always uses `controlId < 0x8000`
(e.g. `CTRL_ROLL`, `CTRL_PITCH`) — these route via HID, never via `sendDcsBiosMessage()`.
The 0–65535 value is converted to ±32767 in the `HIDAxis` lambda on SimGateway (`v - 32768`).
See `07-simgateway-api.md` for the axis declarations.

**I²C init ordering:** `AngleSensor` constructors store the `TwoWire` reference but do not
touch I²C. The sketch must call `Wire.begin()` before `PanelGroup::setup()`, which calls
`sensor.begin()` on each registered sensor.

```cpp
AS5600Sensor  rollSensor(Wire);
MT6701Sensor  pitchSensor(Wire1);

OpenSkyhawk::AngleSensorInput roll (CTRL_ROLL,  rollSensor,  centerDeg, travelDeg);
OpenSkyhawk::AngleSensorInput pitch(CTRL_PITCH, pitchSensor, centerDeg, travelDeg);
```

---

## Output Classes

Output objects are declared at global scope. Constructors self-register. `PanelGroup::loop()`
dispatches each non-null packet in received `CTRL_BCAST` `ControlPacketPair` frames to every
registered output object.

### LED *(exists)*

GPIO pin driven from a single bit of a DCS-BIOS value. Pin HIGH when `(value & mask) != 0`.

```cpp
OpenSkyhawk::LED masterCaution(A_4E_C_MASTER_CAUTION_A, 0x4000, PinRef(PB0));
```

### IntegerOutput *(Not Started — Phase 5)*

User-supplied callback with the raw 16-bit DCS value. Escape hatch for custom output logic.

```cpp
void onCanopyPos(uint16_t v) { /* custom motor drive */ }
OpenSkyhawk::IntegerOutput canopy(A_4E_C_CANOPY_POS_A, onCanopyPos);
```

### AnalogOutput *(new)*

Maps a 16-bit DCS-BIOS value to PWM duty cycle on a direct STM32 GPIO pin. Used for
instrument panel backlighting, gauge backlighting, and floodlights (three independent zones
per MCU board — each gets its own `AnalogOutput`).

**Must be a direct STM32 GPIO pin** — MCP23017 cannot do PWM. Enforced by checking
`pin.isGpio()` in the constructor; sketches still must choose a PWM-capable direct pin.

Value mapping: `dutyCycle = value >> 8` (16-bit → 8-bit). Optional custom scale function as
third argument.

```cpp
OpenSkyhawk::AnalogOutput instrLight(A_4E_C_LIGHTS_INSTRUMENTS_A, PinRef(PB9));
OpenSkyhawk::AnalogOutput floodLight(A_4E_C_LIGHTS_FLOOD_RED_A,   PinRef(PB8));
```

### SwitecX25Output *(new)*

Drives a gauge needle stepper (X27.589 / VID-29 family) via DRV8833PW using the **SwitecX25
library**. Relies on mechanical CCW stop for homing — no physical sensor required.

Homing sequence at boot:
1. `~SLEEP` driven HIGH (motor enabled — required for homing torque).
2. `PanelGroup::setup()` calls `motor.reset()` — steps fully CCW to mechanical stop.
3. After `reset()` completes, needle is at position 0; `~SLEEP` remains HIGH.
4. On first `CTRL_BCAST` frame matching this address, needle moves to received DCS position.

Random motion cannot occur after homing because SwitecX25 only moves on explicit `moveTo()`.

Value mapping: `steps = map(dcsValue, 0, 65535, 0, maxSteps)`. All A-4E gauges are linear.

`SwitecX25Output::update()` must be called every `PanelGroup::loop()` iteration (non-blocking).

**Two motors on one DRV8833:** pass the same `SLEEP_PIN` to both instances.

```cpp
SwitecX25 needle(600, PIN_A1, PIN_A2, PIN_B1, PIN_B2);  // 600 steps = 315°
OpenSkyhawk::SwitecX25Output cabinPressure(A_4E_C_CABIN_ALT_A, needle, SLEEP_PIN);
```

### AccelStepperOutput *(new)*

Drives a gauge needle stepper via **AccelStepper library** with a physical home sensor
(hall-effect or optical switch). For gauges without a mechanical stop or for continuous-rotation
indicators.

Homing: `~SLEEP` HIGH, step toward `homePin` until sensor reads active (LOW), set
`currentPosition = 0`.

```cpp
AccelStepper motor(AccelStepper::FULL4WIRE, PIN_A1, PIN_A2, PIN_B1, PIN_B2);
OpenSkyhawk::AccelStepperOutput gauge(A_4E_C_D_ALT_NEEDLE_A, motor,
                                       HOME_SENSOR_PIN, SLEEP_PIN, minSteps, maxSteps);
```

Value mapping: `steps = map(dcsValue, 0, 65535, minSteps, maxSteps)`.

### ServoOutput *(new)*

Standard RC servo via Arduino **Servo library** (1–2 ms pulse, 50 Hz). Must be a direct STM32
GPIO pin because `Servo.attach()` requires a raw pin. Enforced by checking `pin.isGpio()` and
using `pin.gpioPin()` internally.

Value mapping: `servo.writeMicroseconds(map(dcsValue, 0, 65535, minPulse, maxPulse))`.

```cpp
OpenSkyhawk::ServoOutput flapInd(A_4E_C_D_FLAPS_IND_A, PinRef(SERVO_PIN), 1000, 2000);
```

---

## Wiring Map Convention

All `PinRef` bit positions and mask values must be named constants — no inline literals.
Define a **wiring map** section at the top of each sketch, one named `PinRef` constant per
physical connection, matching the schematic net label. Control declarations use only names.

This means each bit number appears exactly once in the sketch (in the wiring map) and is
directly traceable to the schematic.
