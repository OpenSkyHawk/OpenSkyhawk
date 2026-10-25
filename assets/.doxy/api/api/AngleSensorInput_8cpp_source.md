

# File AngleSensorInput.cpp

[**File List**](files.md) **>** [**AngleSensorInput**](dir_204c2e67ee983494592248b9f39f712d.md) **>** [**AngleSensorInput.cpp**](AngleSensorInput_8cpp.md)

[Go to the documentation of this file](AngleSensorInput_8cpp.md)


```C++

#ifdef ARDUINO_ARCH_STM32

#include "AngleSensorInput.h"

namespace OpenSkyhawk {

namespace {

uint16_t degToCounts(float deg) {
    float wrapped = deg - 360.0f * floorf(deg / 360.0f);   // any input → [0, 360)
    return (uint16_t)(uint32_t)(wrapped * AngleSensorInput::COUNTS_PER_DEG);
}

uint16_t halfSpanCounts(float travelDeg) {
    if (!(travelDeg > 0.0f)) travelDeg = 1.0f;             // also catches NaN
    if (travelDeg > 360.0f)  travelDeg = 360.0f;
    uint32_t half = (uint32_t)(travelDeg * 0.5f * AngleSensorInput::COUNTS_PER_DEG);
    return half > 32767u ? 32767u : (uint16_t)half;
}

// The base's config: the angle's own tuning, plus the raw window the travel implies (centred on
// mid-scale — readRaw() moves the centre angle there).
AnalogConfig baseConfig(float travelDeg, const AngleConfig& cfg) {
    AnalogConfig a;
    a.minRaw     = (uint16_t)(32768u - halfSpanCounts(travelDeg));
    a.maxRaw     = (uint16_t)(32768u + halfSpanCounts(travelDeg));
    a.hysteresis = cfg.hysteresis;
    a.ewmaShift  = cfg.ewmaShift;
    a.pollMs     = cfg.pollMs;
    return a;
}

}  // namespace

// The base's [minRaw, maxRaw] window is centred on mid-scale because readRaw() moves the centre
// angle there — so the window is the same whatever part of the circle the knob sits on.
AngleSensorInput::AngleSensorInput(uint16_t controlId, PinRef pin, float centerDeg,
                                   float travelDeg, const AngleConfig& cfg)
    : AnalogInput(controlId, pin, baseConfig(travelDeg, cfg)),
      _centerRaw(degToCounts(centerDeg)) {}

uint16_t AngleSensorInput::readRaw() {
    // Unsigned wrap is the 0°/360° seam handling: subtracting the centre and adding mid-scale is
    // modulo 65536, so a travel straddling the seam is contiguous like any other.
    return (uint16_t)(AnalogInput::readRaw() - _centerRaw + 32768u);
}

}  // namespace OpenSkyhawk

#endif  // ARDUINO_ARCH_STM32
```


