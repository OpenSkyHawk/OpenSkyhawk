/**
 * @file AngleSensorInput.cpp
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#ifdef ARDUINO_ARCH_STM32

#include "AngleSensorInput.h"

namespace OpenSkyhawk {

namespace {

/** @brief Degrees → counts, wrapped into one turn. Runs once per object, at construction. */
uint16_t degToCounts(float deg) {
    float wrapped = deg - 360.0f * floorf(deg / 360.0f);   // any input → [0, 360)
    return (uint16_t)(uint32_t)(wrapped * AngleSensorInput::COUNTS_PER_DEG);
}

/** @brief Half the travel in counts, clamped so the window stays inside the 16-bit range. */
uint16_t halfSpanCounts(float travelDeg) {
    if (!(travelDeg > 0.0f)) travelDeg = 1.0f;             // also catches NaN
    if (travelDeg > 360.0f)  travelDeg = 360.0f;
    uint32_t half = (uint32_t)(travelDeg * 0.5f * AngleSensorInput::COUNTS_PER_DEG);
    return half > 32767u ? 32767u : (uint16_t)half;
}

}  // namespace

// The base's [minRaw, maxRaw] window is centred on mid-scale because readRaw() moves the centre
// angle there — so the window is the same whatever part of the circle the knob sits on.
AngleSensorInput::AngleSensorInput(uint16_t controlId, PinRef pin, float centerDeg,
                                   float travelDeg, uint16_t pollMs)
    : AnalogInput(controlId, pin, /*reverse=*/false,
                  /*minRaw=*/(uint16_t)(32768u - halfSpanCounts(travelDeg)),
                  /*maxRaw=*/(uint16_t)(32768u + halfSpanCounts(travelDeg)),
                  DEFAULT_HYSTERESIS, DEFAULT_EWMA_SHIFT, pollMs),
      _centerRaw(degToCounts(centerDeg)) {}

uint16_t AngleSensorInput::readRaw() {
    // Unsigned wrap is the 0°/360° seam handling: subtracting the centre and adding mid-scale is
    // modulo 65536, so a travel straddling the seam is contiguous like any other.
    return (uint16_t)(AnalogInput::readRaw() - _centerRaw + 32768u);
}

}  // namespace OpenSkyhawk

#endif  // ARDUINO_ARCH_STM32
