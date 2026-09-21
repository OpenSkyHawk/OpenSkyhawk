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
PanelGroup::registerExpander(exp1, PB12, PB13);  // INTA→PB12, INTB→PB13

// MIRROR mode (same pin for INTA and INTB — PanelGroup sets IOCON.MIRROR):
PanelGroup::registerExpander(exp1, PB12, PB12);  // both ports → PB12

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
| `CTRL_*` (< 0x8000) | SimGateway | `HIDButton` / `HIDAxis` → USB HID report |

Example — same class, different routing:

```cpp
// DCS-BIOS route: master arm switch → sendDcsBiosMessage("ARM_MASTER", "0"/"1")
OpenSkyhawk::Switch2Pos masterArm(DCSIN_ARM_MASTER, PIN_MASTER_ARM);

// HID route: trigger button → HID button 0
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

### Switch2Pos *(implemented)*

Debounced 2-position switch. VALUE: 1 = active (pin LOW), 0 = inactive. Debounce: 20 ms fixed.

```cpp
OpenSkyhawk::Switch2Pos masterArm(DCSIN_ARM_MASTER, PinRef(PB5));
OpenSkyhawk::Switch2Pos ejSafe   (DCSIN_SEAT_EJECT_SAFE, PinRef(expander1, PORT_A, 3));
```

### Switch3Pos *(implemented)*

3-position switch (ON-OFF-ON or ON-ON). VALUE: 0 = pin A active, 1 = neither (centre),
2 = pin B active. Debounce: 20 ms per state.

If both pins read active simultaneously (hardware fault or bounce during throw — mechanically
impossible on a 3-position switch), **pin A takes priority** and VALUE 0 is reported, matching
DcsBios `Switch3Pos` (`readState` checks pin A first). The 20 ms debounce absorbs the transient
regardless.

```cpp
OpenSkyhawk::Switch3Pos fuelSelector(DCSIN_FUEL_SEL,
                                      PinRef(expander1, PORT_A, 0),
                                      PinRef(expander1, PORT_A, 1));
```

### SwitchMultiPos *(implemented)*

Multi-position rotary selector switch. N discrete pins — exactly one active (LOW) at a time.
VALUE: position index 0 to N-1.

If no pin reads active, last valid state is retained.

Supports `PIN_NC` sentinel for positions with no physical pin (mechanical-only detents).

```cpp
const PinRef weaponPins[] = { PinRef(expander1, PORT_B, 0), PinRef(expander1, PORT_B, 1),
                               PinRef(expander1, PORT_B, 2), PinRef(expander1, PORT_B, 3) };
OpenSkyhawk::SwitchMultiPos weaponSel(DCSIN_WEAPON_SEL, weaponPins, 4);
```

### AnalogMultiPos *(implemented)*

Resistor-ladder multi-position selector. Single analog `PinRef` reads a different voltage per
position. VALUE: position index 0 to N-1 (same as `SwitchMultiPos`). Both send an absolute value on
`EVT_n` (ABS — PanelBridge formats `%u` → `set_state`); no SimGateway declarations needed.

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

### ActionButton

Momentary push button driving a control that **latches in the sim**. Emits one `EVT_ACTION_n` frame
on the press edge and nothing on release. Payload value is a selector, not a magnitude: `0` means
`TOGGLE`, which PanelBridge renders as the DCS-BIOS action argument. The DCS-BIOS string never
appears node-side.

```cpp
OpenSkyhawk::ActionButton armMaster(DCSIN_ARM_MASTER, PinRef(exp1, PORT_A, 5));
OpenSkyhawk::ActionButton apcEnable(DCSIN_APC_ENABLE, PinRef(PB12), /*reverse=*/true);
```

**Use it only for controls DCS-BIOS defines with `defineToggleSwitch`.** `TOGGLE` reads the
control's current sim value, flips it, and writes it back — so each press toggles, the state
persists, and the panel cannot desync from the sim. Pointing it at a `definePushButton` control is
wrong: those are `momentary_last_position`, and a TOGGLE latches them on until the next press. Use
`Switch2Pos` for those.

Compared with `Switch2Pos`, which sends absolute 0/1 and therefore needs a physical latching switch:
`ActionButton` lets a *momentary* part drive a latching sim control, and cannot fall out of sync the
way a physical switch can after a cold start, keyboard binding, or mission script. The trade-off is
no at-a-glance state.

Debounce: 20 ms of post-fire suppression, not a stability window — an action's first edge *is* the
event, so waiting for the level to settle would only add latency. Holding the button emits exactly
once; the press edge is consumed and no further state change occurs until release.

`forceReport()` **emits nothing** — an action is not idempotent, so emitting on the boot burst and
every `SYNC_REQ` would flip the sim switch each time. It still seeds the baseline, which is what
stops a button held at boot from reading as a press edge on the first `poll()`.

### RotaryEncoder *(implemented — #117, REL/DIR #147)*

Quadrature encoder (A/B pins) — a **relative** control: it reports motion, never an absolute
position. Ports `DcsBios::RotaryEncoder`'s transition table: the delta accumulates and one detent
fires when `|delta| >= stepsPerDetent` (`EncoderStepsPerDetent::One` / `Two` / `Four` / `Eight`).
Direction travels in the **sign** of the value; the mode picks the DCS-BIOS interface and the CAN
frame:

| `EncoderMode` | DCS-BIOS interface | Frame | Value per detent | PanelBridge sends |
|---|---|---|---|---|
| `Rel` (default) | `variable_step` — continuous knobs (lat/lon, baro, heading bug) | `canIdEvtRel` `0x500+n` | `±step` (default 3200 = DCS `suggested_step`) | `%+d`, e.g. `+3200` |
| `Dir` | `fixed_step` — bounded selectors with no pointer (ARC-51 frequency) | `canIdEvtDir` `0x600+n` | `±1` (anything else is dropped) | `INC` / `DEC` |

Both modes are preset-safe: `forceReport()` resyncs the Gray state and emits nothing, so boot and
`SYNC_REQ` never clobber a mission preset — DCS owns the position (and clamps DIR selectors at their
ends). REL coalesces a burst into one frame (`detents × step`); DIR sends one frame per detent.

Read model: `poll()` decodes the cached A/B bits at loop rate (MCP23017 bits are refreshed on the
expander interrupt, not a per-encoder ISR). On a ShiftBus node built with `SHIFTBUS_ISR_HZ`, a timer
ISR calls `sampleTick()` to decode at kHz rate and `poll()` only drains pending detents.

```cpp
OpenSkyhawk::RotaryEncoder destLat(DCSIN_DEST_LAT_KNB,
                                    PinRef(expander2, PORT_A, 6), PinRef(expander2, PORT_A, 7),
                                    EncoderStepsPerDetent::Four);                     // REL, ±3200
OpenSkyhawk::RotaryEncoder freq10(DCSIN_ARC51_FREQ_10MHZ,
                                   PinRef(expander2, PORT_B, 0), PinRef(expander2, PORT_B, 1),
                                   EncoderStepsPerDetent::Four, EncoderMode::Dir);  // INC / DEC
```

### RotaryAcceleratedEncoder *(implemented — #287)*

`DcsBios::RotaryAcceleratedEncoder` parity as a **thin subclass of `RotaryEncoder`** (D16). The
base class's public constructor stays the plain `DcsBios::RotaryEncoder` equivalent; the subclass
adds, through a protected base constructor:

- **Momentum filter** — the DCS-BIOS class's stated purpose ("noisy/faulty rotaries"). Momentum
  builds to ±(4 × stepsPerDetent) transitions; a transition against it is dropped and only decays
  it; momentum clears after 500 ms without a detent.
- **Speed** — a detent less than 175 ms after the previous one adds `fastStep` instead of `step`.
  Two-tier, not a ramp — same as DCS-BIOS.

Speed is classified **per detent inside `decode()`** (which may run in the ShiftBus ISR), not when
pending detents are drained — a stalled loop would otherwise erase the timing. REL pending becomes a
counter (`_pendingFast`, beside the untouched `_pendingDetents`), and a burst of slow + fast detents
drains as one REL frame carrying their sum. No wire, bridge or map change: a fast detent is just a
bigger `±step` on the REL frame (supersedes D9's 4-value scheme). `fastStep = 0` turns the speed-up
off; the first detent after a resync is always slow. Header-only, in its own
`Inputs/RotaryAcceleratedEncoder/` folder.

Two constructors:

```cpp
// REL — filter + speed-up
OpenSkyhawk::RotaryAcceleratedEncoder pposLat(DCSIN_PPOS_LAT_KNB,
    PinRef(exp1, PORT_B, 0), PinRef(exp1, PORT_B, 1),
    EncoderStepsPerDetent::Four, /*step*/ 3200, /*fastStep*/ 12800);
// DIR — filter only (the DIR wire is strictly ±1, so there is no speed to add)
OpenSkyhawk::RotaryAcceleratedEncoder freq1(DCSIN_ARC51_FREQ_1MHZ,
    PinRef(exp1, PORT_B, 2), PinRef(exp1, PORT_B, 3),
    EncoderStepsPerDetent::Four, EncoderMode::Dir);
```

### RotarySwitch *(dropped — D16)*

Not ported. `DcsBios::RotarySwitch` assumes position 0 at boot and sends absolute `set_state`
values, so the first click after a cold start can jump the sim away from a mission preset (it only
realigns when turned to an end stop). Use `RotaryEncoder` in `EncoderMode::Dir` for bounded
selectors: it sends INC/DEC, DCS owns the position and clamps at the ends, and the cockpit readout
(e.g. the ARC-51 drums) shows the result — the pattern DCS-BIOS's own M2000C radio examples use.

### AnalogInput *(implemented)*

Continuous or stepped analog input. All sources normalised to **16-bit (0–65535)**:

| Source | Raw resolution | Conversion | Notes |
|--------|---------------|------------|-------|
| STM32 ADC | 12-bit (0–4095) | `analogReadResolution(16)` — framework scales → 0–65520 | Set in `STM32Board::begin()` |
| ADS1115 | 15-bit single-ended (0–32767) | ×2 → 16-bit (0–65534) | GAIN_ONE (±4.096V FSR); 3.3V → ~52800 |

Configurable `[minRaw, maxRaw]` input range; values outside clamped to 0 or 65535. Read throttle:
**per instance** — `pollMs`, default `DEFAULT_POLL_MS` = 8 ms, so a HID flight axis and a cockpit
pot on the same node can sample at different rates (`forceReport()` bypasses the throttle for the
boot / SYNC baseline).

**Filtering — ports DcsBios `PotentiometerEWMA`:** an integer EWMA low-pass (α = 1/2^`ewmaShift`, a
shift not a divide — no soft-float on the F103) smooths the ×16-amplified ADC noise; a new value is
emitted only when the smoothed result moves more than `hysteresis` counts from the last sent value,
or reaches a rail (0 / 65535) moving toward it — so endpoints are always reached and a settled pot
is silent. The constructor exposes `reverse`, `[minRaw, maxRaw]`, `hysteresis` (default 128),
`ewmaShift` (default 3; **valid 0–15**, capped so the `int32` accumulator `scaled << ewmaShift`
cannot overflow at full scale), and `pollMs` (default 8; **not** capped — 0 means read every loop
iteration). `pollMs` and `ewmaShift` are **coupled**: the filter time constant is
τ ≈ 2^`ewmaShift` × `pollMs`, so halving the read interval halves τ unless the shift rises with it.
A *linear* class — it sends an absolute value on `EVT_n` (ABS) like the selector classes, but the
value is continuous, not a position index.

```cpp
OpenSkyhawk::AnalogInput throttle(CTRL_THROTTLE, PinRef(PA0));

ADS1115 adc(0x48, Wire);
OpenSkyhawk::AnalogInput rudder(CTRL_RUDDER, PinRef(adc, 0));
```

The 16-bit value is used as-is by both routing paths: PanelBridge passes it to
`sendDcsBiosMessage()` for DCS-BIOS controls (ABS, `%u` → `set_state`); SimGateway passes it to
`HIDAxis::dispatch()` for joystick axes — no rescaling at either destination.

### AngleSensorInput *(planned — #294, Firmware v0.1.0)*

A magnetic angle sensor (AS5600 / MT6701) as an **absolute** knob or axis — no wiper wear and a full
360° mechanical range, where a pot stops at ~270°. Use case: absolute DCS-BIOS knobs such as
`GUNSIGHT_KNB` (gunsight elevation) or `RADAR_RETICLE`; flight-control axes use linear Hall sensors
on `AnalogInput` instead (#278).

Design (D16) — **`AngleSensorInput : AnalogInput`**, not a sibling class:

- **Takes a `PinRef`** like every control class: the sensor's analog output pin, read through an
  STM32 ADC pin or an ADS1115 channel. It calls `AnalogInput`'s existing public constructor, with
  `centerDeg` / `travelDeg` converted to `[minRaw, maxRaw]`.
- `AnalogInput` gains one protected virtual `readRaw()` (default: `_pin.readAnalog()`);
  `AngleSensorInput` overrides it to **re-centre** the reading so `centerDeg` lands at mid-scale —
  the 0°/360° seam then only matters for travel wider than ±180°. Everything else is inherited —
  EWMA, hysteresis, per-instance `pollMs`, and **routing by `controlId`**: `DCSIN_*` → ABS
  `set_state` through PanelBridge, `CTRL_*` → HID through SimGateway.
- Reading the angle register over I²C (higher resolution, no ADC noise) is a later **`PinRef`
  backend** (`PinRef(as5600)`, like the ADS1115's), carrying its own `I2cHealth` + `FaultSource`
  contract — not an object passed to this class.
- A DCS knob that only accepts relative input (`variable_step` only) would need a syncing mode like
  `DcsBios::RotarySyncingPotentiometer` (read DCS's value back, send `%+d` corrections) — a later
  addition, not part of the first cut.

```cpp
ADS1115 adc(0x48, Wire);
// AS5600 OUT pin → ADS1115 channel 2 (or an STM32 ADC pin: PinRef(PA3))
OpenSkyhawk::AngleSensorInput gunsight(DCSIN_GUNSIGHT_KNB, PinRef(adc, 2),
                                        /*centerDeg*/ 180.0f, /*travelDeg*/ 150.0f);
```

---

## Output Classes

Output objects are declared at global scope. Constructors self-register. `PanelGroup::loop()`
dispatches each non-null packet in received `CTRL_BCAST` `ControlPacketPair` frames to every
registered output object.

### LED *(implemented)*

GPIO pin driven from a single bit of a DCS-BIOS value. Pin HIGH when `(value & mask) != 0`.

```cpp
OpenSkyhawk::LED masterCaution(A_4E_C_MASTER_CAUTION_A, 0x4000, PinRef(PB0));
```

### AnalogOutput family *(planned — #288, Firmware v0.1.0)*

One 16-bit DCS-BIOS value driving something proportional (D16). **`AnalogOutput`** is an abstract
`OutputBase`: it owns `controlId` + mask/shift matching (like `LED`) and change dedup, and hands
the value to a protected virtual `apply()`. Subclasses only change the sink:

| Class | Sink | DCS-BIOS equivalent |
|---|---|---|
| `Dimmer` | PWM duty on a **direct STM32 GPIO timer pin** — the backlight zone MOSFET gate | `DcsBios::Dimmer` |
| `IntegerOutput` | a user callback with the value — custom displays, LCDs, anything bespoke | `DcsBios::IntegerBuffer` |
| `ServoOutput` *(optional, later)* | servo pulse width, for non-gauge uses (a flag, a lever) | `DcsBios::ServoOutput` |

Needles stay on `NeedleGauge` + a `MotorDriver` (`ServoMotor`, #132) — they need `GaugeCal` and
smooth motion, which a plain `ServoOutput` doesn't give.

**`Dimmer`** — the A-4E-C's five light-intensity outputs (`LIGHTS_CONSOLE`, `LIGHTS_INSTRUMENTS`,
`LIGHTS_FLOOD_RED`, `LIGHTS_FLOOD_WHITE`, `APG53A_GLOW`). Takes a `PinRef` like every control class
and **checks it**: PWM comes from a timer channel, so `configure()` requires a direct GPIO on one —
an expander / ShiftBus pin or a GPIO without a timer is logged and the output stays disabled. It
writes through `PinRef::writeAnalog()`. Duty is 0 at `configure()` so a zone stays dark until the
first matching frame. Default map `duty = value >> 8`; an optional scale function
covers perceptual curves or inversion. On the PanelGroup base the two zones are PA6 / PA7.

```cpp
OpenSkyhawk::Dimmer instrLights(A_4E_C_LIGHTS_INSTRUMENTS, PinRef(PA6));   // TIM3_CH1 → J_BL1
OpenSkyhawk::Dimmer floodRed   (A_4E_C_LIGHTS_FLOOD_RED,   PinRef(PA7));   // TIM3_CH2 → J_BL2

void onCanopyPos(uint16_t v) { /* custom drive */ }
OpenSkyhawk::IntegerOutput canopy(A_4E_C_CANOPY_POS, onCanopyPos);
```

### NeedleGauge *(implemented)*

Drives a **needle / pointer gauge** from one DCS-BIOS address. `NeedleGauge` is a thin `OutputBase`
that does only the **gauge semantics** — decode the 16-bit value, map it to a motor position (linear
or piecewise-calibrated via `GaugeCal`), and command the motor. All low-level drive, acceleration,
and homing live in a **`MotorDriver`** the gauge *composes*, so the ~119 A-4E pointer gauges share
one class over any backend.

```cpp
struct GaugeCal {                       // motor positions are driver-native units (steps)
    int16_t minTravel, maxTravel;       // positions at DCS value 0 / 65535 (linear path)
    bool reverse;                       // flip direction (mounted / wired reversed)
    const uint16_t *curveIn, *curveOut; // ascending breakpoints + positions, or nullptr = linear
    uint8_t curveN;                     // breakpoint count (0 = linear)
};

NeedleGauge(uint16_t controlId, uint16_t mask, MotorDriver& motor, const GaugeCal& cal);
```

- **Linear** (`curveN == 0`): `pos = map(value, 0, 65535, minTravel, maxTravel)`; signed travel lets
  a centre-zero gauge (DRIFT) sit mid-range. **Piecewise** (`curveN ≥ 2`): binary-search `curveIn`,
  interpolate `curveOut` (non-linear dials). `reverse` flips the input before either path.
- `onControlPacket()` **stores only** — computes the target and calls `motor.moveTo()`, never steps;
  the coil drive happens in `update()` (off the packet path, like `LED`). `configure()` homes the
  motor (may block — boot only). `update()` runs every `PanelGroup::loop()` iteration (non-blocking).

#### Motor-driver layer — `PanelGroup/Drivers/`

The backend is a **`MotorDriver`** the sketch builds and passes by reference (composition, like
`DrumDisplay` taking a `U8G2&`) — *not* an enum inside the gauge. Today: **`StepperMotor`** (integer
SwitecX25-style acceleration; drives four coils through `PinRef` — native GPIO **or** MCP23017;
homing by **mechanical STALL** or a **home sensor**; one air-core profile covers X27.589 / VID-29 /
BKA-30, run at **5 V through a DRV8833**). A **`ServoMotor`** backend is planned (#132). See
`08-hardware-firmware-contracts.md` for the DRV8833 `~SLEEP` contract.

```cpp
StepperMotor driftMotor(PinRef(PA0), PinRef(PA1), PinRef(PA4), PinRef(PA5), DRIFT_MOTOR_CFG);
const GaugeCal DRIFT_CAL = { -150, 150, false, nullptr, nullptr, 0 };  // centre-zero, ±150 steps
OpenSkyhawk::NeedleGauge drift(A_4E_C_APN153_DRIFT_GAUGE, A_4E_C_APN153_DRIFT_GAUGE_AM,
                               driftMotor, DRIFT_CAL);
```

**Supersedes** the former `SwitecX25Output` / `AccelStepperOutput` / `ServoOutput` standalone classes
— the backend is now a swappable `MotorDriver`, not a per-library output class.

---

## Wiring Map Convention

All `PinRef` bit positions and mask values must be named constants — no inline literals.
Define a **wiring map** section at the top of each sketch, one named `PinRef` constant per
physical connection, matching the schematic net label. Control declarations use only names.

This means each bit number appears exactly once in the sketch (in the wiring map) and is
directly traceable to the schematic.
