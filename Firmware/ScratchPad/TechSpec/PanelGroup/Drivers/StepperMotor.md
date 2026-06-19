# StepperMotor — Technical Specification

**Status:** Done
**FirmwarePlan ref:** `FirmwarePlan/05-panelgroup-api.md#switecx25output-new`, `FirmwarePlan/08-hardware-firmware-contracts.md#drv8833pw--sleep-pin-contract`
**Depends on:** `PanelGroup.md`, `PinRef.md`, `Drivers/MotorDriver.md`
**GitHub:** #122

---

## Responsibility

A `MotorDriver` for instrument gauge steppers. Drives four coils through `PinRef`, so the coils
may be native STM32 GPIO **or** an MCP23017 expander with no code change. The motion engine is the
integer (no-FPU) table-driven trapezoidal accel/decel ported from Guy Carpenter's **SwitecX25**
library. Owns homing (mechanical stop or digital sensor), an anti-jitter deadband, continuous-rotation
wrap, and optional sensor auto-recalibration.

Does **not** decode DCS-BIOS or map values — that is `NeedleGauge`'s job. Does **not** self-register
on the OutputBase list — it is ticked by its owning control.

One drive profile (`SWITEC_6STATE`) covers the air-core family on hand: **X27.589 / VID-29 / BKA-30
are the same motor electrically.** A coil that runs reversed is corrected by swapping two constructor
pins (the AHN "BKA-30 reorder"), **not** a separate profile. `FULL_4STATE` is reserved for generic
geared 4-wire steppers.

---

## File Layout

```
Firmware/Libraries/PanelGroup/
└── Drivers/StepperMotor/StepperMotor.{h,cpp}
```

No new 3rd-party dependency (own PinRef stepping) — so it stays inside PanelGroup, safe under the
`deep+` LDF, and adds **0 bytes** to nodes that don't instantiate it (verified flash-Δ on
`Center_Armament`).

### Test project — `Firmware/Tests/StepperMotor/`

| Scenario | Type | Verifies |
|---|---|---|
| `motion_profile` | CI | accel ramp up→down, top-speed clamp, **stop-at-target no overshoot**, triangular short move, reversal |
| `home_stall`     | CI | STALL homing → `homed()`, parks at `parkPosition` |
| `home_sensor`    | CI | active-LOW/HIGH polarity, debounce, `maxSeekSteps` abort (no hang), successful home+park |
| `deadband`       | CI | sub-deadband target changes ignored |
| `wrap`           | CI | continuous-rotation shortest signed path; `position()` wraps |
| `step_pattern`   | CI | 6-state and 4-state both reach target |
| `bringup`        | bench | sweep a real X27 on 4 GPIO at 3.3 V |
| `mcp23017`       | bench | coils on an MCP23017 expander |
| `cal_steps_per_rev` | bench | empirical steps/rev via a zero sensor (AHN method) |
| `motor_compat`   | bench | X27 / VID-29 / BKA-30 all on the one profile |
| `accuracy_sweep` | bench | step-rate envelope + missed-step / re-home drift |

CI scenarios are deterministic via `-DSTEPPERMOTOR_TEST` accessors (`debugAdvance()`,
`debugCurrentStep()`, `debugVel()`, `debugMicroDelay()`, `debugStopped()`, `debugSensorAsserted()`,
`debugSetSensorOverride()`) — driven with NC coils, no motor required.

---

## Public API

```cpp
namespace OpenSkyhawk {

enum class HomeMode    : uint8_t { STALL, SENSOR };                 // ABSOLUTE = backlog
enum class StepPattern : uint8_t { SWITEC_6STATE, FULL_4STATE };

struct AccelPoint { uint16_t stepThreshold; uint16_t delayUs; };    // SwitecX25 (cumstep → delay)
struct HomeSensor { bool activeLow; uint8_t debounceMs; uint16_t maxSeekSteps; };

struct StepperConfig {
    uint16_t          stepsPerRev;       // calibrate empirically (cal_steps_per_rev)
    StepPattern       pattern;
    const AccelPoint* accel; uint8_t accelN;   // last delayUs = top speed
    HomeMode          home;
    bool              homeSeekClockwise;
    HomeSensor        sensor;             // SENSOR only
    int16_t           homePosition;       // step index at the home reference
    int16_t           parkPosition;       // rest position after homing
    int16_t           minPos, maxPos;     // moveTo clamp (ignored if wrap)
    bool              wrap;               // continuous-rotation, shortest path
    uint8_t           deadband;
    bool              autoRecal; uint32_t recalDebounceMs;
};

extern const AccelPoint kSwitecDefaultAccel[5];   // {20,3000},{50,1500},{100,1000},{150,800},{300,600}
constexpr uint8_t       kSwitecDefaultAccelN = 5;

class StepperMotor : public MotorDriver {
public:
    StepperMotor(PinRef c1, PinRef c2, PinRef c3, PinRef c4, const StepperConfig& cfg,
                 PinRef homeSense = PinRef(), PinRef sleepEn = PinRef());
    void    configure() override;   void home() override;
    void    moveTo(int32_t) override; void update() override;
    int32_t position() const override;
    bool    homed() const;
};

} // namespace OpenSkyhawk
```

---

## Key Data Structures

- **`AccelPoint[]` accel table** — the tuning knob. `vel` (steps under acceleration since rest) indexes
  it; the matching `delayUs` is the inter-step delay. Last `delayUs` sets top speed. The three air-core
  motors share `kSwitecDefaultAccel`; a higher-torque geared motor gets its own (gentler) table.
- **Coil state tables** (private): `SWITEC_6STATE = {0x9,0x1,0x7,0x6,0xE,0x8}`,
  `FULL_4STATE = {0x5,0x6,0xA,0x9}`. Bit *i* (LSB-first) drives coil *i*.

---

## Implementation Notes

### Motion engine (ported from `SwitecX25::advance`)

`update()` is the non-blocking gate: `if (!stopped && micros()-t0 ≥ microDelay) advance()`. `advance()`
steps one detent toward the target, then adjusts `vel`:
`if (stepsToTarget < vel) vel--` (decelerate into the target) `else if (vel < maxVel) vel++`
(accelerate) `else` cruise; moving away → `vel--`. `vel` then indexes the accel table for the next
delay. Integer-only — no FPU. Per SwitecX25, the needle may dither ≤1–2 steps onto the target before
settling exactly (`currentStep == targetStep && vel == 0`).

### Homing (reduced speed for accuracy)

- **STALL** — step into the mechanical end-stop `stepsPerRev` times (a full revolution seats from any
  start), set `pos = homePosition`. No sensor.
- **SENSOR** (two-pass, à la AHN) — clear the sensor, seek `homeSeekClockwise` until the debounced read
  asserts, hard-stop, set `pos = homePosition`; back off and re-seek to refine; then move to
  `parkPosition`. `maxSeekSteps` aborts a never-asserting (mis-wired) sensor → `homed() == false`, no
  hang. **Sensor-agnostic:** micro switch / reed / hall / opto all reduce to one debounced level read;
  `activeLow` sets polarity. Sensor read via `PinRef::read()` (native GPIO immediate; MCP23017 = the
  ~20 ms PanelGroup cache).

### Coil drive & MCP23017

Each coil write is `PinRef::write()` → native `digitalWrite` or `PanelGroup::writeCachedPin` (one I2C
transaction per coil). The expander path is ≈ 240 µs/step → smooth at moderate speed but caps fast
sweeps; quantify per board with `accuracy_sweep`.

### Other

- **moveTo** clamps to `[minPos, maxPos]` (or computes the shortest signed path when `wrap`), then
  applies `deadband` (ignore target changes within N steps of the current target — anti-jitter).
- **auto-recal** — when `autoRecal` and the home sensor next asserts (≥ `recalDebounceMs` since the
  last), re-zero `currentStep = homePosition`; the in-flight move continues, cancelling accumulated
  step error.
- **`sleepEn`** (optional) is driven HIGH in `configure()` to enable a driver IC's `~SLEEP`/enable;
  NC for bare air-core drive at 3.3 V.

---

## Dependencies

| Dependency | Source | Notes |
|---|---|---|
| PanelGroup | `Firmware/Libraries/PanelGroup` | `PinRef`, MCP23017 write/read cache |
| MotorDriver | `PanelGroup/Drivers/MotorDriver` | abstract base |

No external stepper library — the SwitecX25 *algorithm* is ported (BSD-2), not its code, because the
stock library does direct GPIO `digitalWrite` and cannot target an MCP23017.
