# NeedleGauge — Technical Specification

**Status:** Done
**FirmwarePlan ref:** `FirmwarePlan/05-panelgroup-api.md#switecx25output-new`, `FirmwarePlan/08-hardware-firmware-contracts.md#drv8833pw--sleep-pin-contract`
**Depends on:** `PanelGroup.md`, `PinRef.md`, `Drivers/MotorDriver.md`, `Drivers/StepperMotor.md`
**GitHub:** #122

---

## Responsibility

Drives a **needle / pointer gauge** from one DCS-BIOS address. `NeedleGauge` is a thin
`OutputBase` that does only the **gauge semantics** — decode the 16-bit value, map it to a
motor position (linear or piecewise-calibrated), and command the motor. All low-level drive,
acceleration, and homing live in a **`MotorDriver`** the gauge *composes* (see
`Drivers/MotorDriver.md`), so the 119 A-4E pointer gauges share one class over any backend.

Does **not** energise coils, generate steps, accelerate, or home — that is the `MotorDriver`'s
job. Does **not** know the motor type. Does **not** interpret what the DCS-BIOS address means.

**Supersedes** the former `SwitecX25Output` / `ServoOutput` / `AccelStepperOutput` standalone
specs: the "backend" is now a `MotorDriver` chosen by the sketch (a `StepperMotor` today; a
`ServoMotor` / `StepDirMotor` later), not an enum inside this class.

---

## File Layout

```
Firmware/Libraries/PanelGroup/
└── Outputs/NeedleGauge/NeedleGauge.{h,cpp}
```

### Test project — `Firmware/Tests/NeedleGauge/`

| Scenario | Verifies |
|---|---|
| `value_map`   | linear `valueToPos`: 0→minTravel, 65535→maxTravel, midpoints, `reverse` flip |
| `calibration` | piecewise curve: breakpoints exact, linear interpolation within segments |
| `drift`       | APN-153 `A_4E_C_APN153_DRIFT_GAUGE` decode + centre-zero + end-to-end packet→motor target, controlId filter |

`-DNEEDLEGAUGE_TEST` exposes `debugValueToPos()`; `-DSTEPPERMOTOR_TEST` exposes the motor's
`debugTargetStep()` for the end-to-end check. Mapping is asserted deterministically — no motor
motion required.

---

## Public API

```cpp
// NeedleGauge.h
#include <PanelGroup.h>                       // OutputBase
#include <Drivers/MotorDriver/MotorDriver.h>  // MotorDriver

namespace OpenSkyhawk {

struct GaugeCal {
    int16_t         minTravel;  // motor position at DCS value 0      (linear path)
    int16_t         maxTravel;  // motor position at DCS value 65535  (linear path)
    bool            reverse;    // flip direction (mounted/wired reversed)
    const uint16_t* curveIn;    // ascending DCS breakpoints, or nullptr for linear
    const uint16_t* curveOut;   // matching positions for curveIn
    uint8_t         curveN;     // breakpoint count (0 = linear)
};

class NeedleGauge : public OutputBase {
public:
    NeedleGauge(uint16_t controlId, uint16_t mask, MotorDriver& motor, const GaugeCal& cal);
    void configure() override;                                  // motor.configure() + motor.home()
    void onControlPacket(uint16_t controlId, uint16_t value) override;  // store-only: motor.moveTo()
    void update() override;                                     // motor.update()
};

} // namespace OpenSkyhawk
```

---

## Key Data Structures

`GaugeCal` (above). Two mapping modes:
- **Linear** (`curveN == 0`): `pos = map(value, 0, 65535, minTravel, maxTravel)`. `minTravel`/
  `maxTravel` are signed, so a centre-zero gauge (DRIFT) sits mid-range (value 32768 → 0).
- **Piecewise** (`curveN ≥ 2`): binary-search the segment in `curveIn`, linearly interpolate
  the matching `curveOut`. For non-linear dials (airspeed, VVI) — ported from OpenHornet's
  `multiMap`. `curveOut` is unsigned (non-linear dials use a positive range).

`reverse` flips the input (`65535 - value`) before either path, reversing both modes uniformly.

`minTravel`/`maxTravel`/`curveOut` are in **driver-native units** (steps for `StepperMotor`).
A sketch may author them as degrees and convert `round(deg·stepsPerRev/360)`.

---

## Implementation Notes

- **`onControlPacket()` stores only** — it computes the target and calls `motor.moveTo()`, never
  stepping. The slow coil-drive happens in `update()` (mirrors `LED.cpp`'s off-the-packet-path
  rule). The `mask` is applied before mapping; gauges normally use `0xFFFF` (whole-word value).
- **`configure()` homes the motor** (`motor.configure()` then `motor.home()`), so the needle has
  a known zero before the first packet. Homing may block (boot only).
- **Composition, not inheritance** — the sketch builds the `MotorDriver` (with its pins, accel
  table, homing mode) and passes it by reference, exactly like `DrumDisplay` takes a `U8G2&`.
  This is what lets one class serve every backend without per-backend bloat.

---

## Sketch Usage (APN-153 DRIFT)

```cpp
#include <OpenSkyhawk.h>          // umbrella: PanelGroup + StepperMotor + NeedleGauge + LED
#include <A4EC_OutputIds.h>
using namespace OpenSkyhawk;

static const AccelPoint DRIFT_ACCEL[] = { {20,3000},{50,1500},{100,1000},{150,800},{300,600} };
static const StepperConfig DRIFT_MOTOR = {
    /*stepsPerRev*/720, StepPattern::SWITEC_6STATE, DRIFT_ACCEL, 5,
    HomeMode::STALL, /*homeSeekCW*/false, /*sensor*/{false,0,0},
    /*home*/0, /*park*/0, /*minPos*/-200, /*maxPos*/200,
    /*wrap*/false, /*deadband*/1, /*autoRecal*/false, /*recalMs*/0,
};
StepperMotor driftMotor(PinRef(PA0), PinRef(PA1), PinRef(PA4), PinRef(PA5), DRIFT_MOTOR);

static const GaugeCal DRIFT_CAL = { -150, 150, false, nullptr, nullptr, 0 };
NeedleGauge drift(A_4E_C_APN153_DRIFT_GAUGE, A_4E_C_APN153_DRIFT_GAUGE_AM, driftMotor, DRIFT_CAL);

// PanelGroup::setup() calls configure() (homes the needle); PanelGroup::loop() ticks update().
```

---

## Dependencies

| Dependency | Source | Notes |
|---|---|---|
| PanelGroup | `Firmware/Libraries/PanelGroup` | OutputBase, CTRL_BCAST dispatch |
| MotorDriver / StepperMotor | `PanelGroup/Drivers/` | the composed backend (no heavy 3rd-party dep) |
| A4EC | `Firmware/Libraries/A4EC` | `A_4E_C_*` address + `_AM` mask constants (sketch only) |
