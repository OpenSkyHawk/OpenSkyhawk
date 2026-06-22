/**
 * @file AnalogMultiPos.h
 * @brief Resistor-ladder multi-position selector for OpenSkyhawk PanelGroup nodes.
 *
 * @version 0.1.0
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <PanelGroup.h>                          // PinRef
#include <Inputs/MultiPosInput/MultiPosInput.h>  // MultiPosInput base

namespace OpenSkyhawk {

/** @brief posVals[] sentinel: a position with no physical detent (no distinct voltage). */
static constexpr uint16_t ANALOG_NC = 0xFFFF;

/**
 * @brief Resistor-ladder multi-position selector — one analog `PinRef`, a different voltage per
 *        position. Emits the resolved position index 0..N-1 over CAN (MULTIPOS dispatch).
 *
 * @details Subclass of `MultiPosInput` — it shares the debounce / emit-on-change / hold-last /
 * forceReport contract and provides only the analog read: it maps the 16-bit ADC reading to a
 * position via detection bands centred on each position's expected value.
 *
 * Two constructions:
 *  - **explicit ladder** — pass `posVals[]`, the expected 16-bit ADC value per position. A
 *    position whose entry is `ANALOG_NC` has no detent and is never emitted; its neighbours'
 *    bands span its place.
 *  - **equal-spacing shorthand** — pass only N; positions are evenly spaced 0..65535.
 *
 * Detection bands: each position's band reaches half-way to its nearest *valid* neighbours, minus
 * `deadband` counts on each edge (default 1000). A reading in the deadband gap between two bands
 * resolves to `NO_POSITION`, so the base holds the last position — this gives switch hysteresis,
 * no flicker at a boundary. The ADC is re-read at most every `POLL_MS` (8 ms).
 *
 * The base debounce window is 0: the deadband gaps provide the filtering, not a timer.
 */
class AnalogMultiPos : public MultiPosInput {
public:
    static constexpr uint16_t DEFAULT_DEADBAND = 1000;  ///< counts trimmed from each band edge
    static constexpr uint16_t POLL_MS          = 8;     ///< min interval between ADC reads (ms)

    /**
     * @brief Explicit resistor-ladder selector.
     *
     * @param controlId  DCSIN_* or CTRL_* constant. Determines PanelBridge routing.
     * @param pin        analog PinRef (STM32 ADC GPIO or ADS1115 channel).
     * @param numPos     N — number of positions (valid indices 0..N-1).
     * @param posVals    caller-owned array of N expected 16-bit ADC values, one per position.
     *                   Must outlive this object. Use ANALOG_NC for a position with no detent.
     * @param deadband   counts trimmed from each band edge for hysteresis (default 1000).
     */
    AnalogMultiPos(uint16_t controlId, PinRef pin, uint8_t numPos,
                   const uint16_t* posVals, uint16_t deadband = DEFAULT_DEADBAND);

    /**
     * @brief Equal-spacing shorthand — positions evenly spaced across the full ADC range.
     *
     * @param controlId  DCSIN_* or CTRL_* constant.
     * @param pin        analog PinRef.
     * @param numPos     N — number of positions.
     * @param deadband   counts trimmed from each band edge (default 1000).
     */
    AnalogMultiPos(uint16_t controlId, PinRef pin, uint8_t numPos,
                   uint16_t deadband = DEFAULT_DEADBAND);

    /** @brief Configure the pin as an input. Called by PanelGroup::setup(). */
    void configure() override;

#ifdef ANALOGMULTIPOS_TEST
    /** @brief Test seam — resolve a raw 16-bit ADC value to an index (bypasses ADC + throttle). */
    uint16_t debugResolve(uint16_t raw) const { return resolve(raw); }
    /** @brief Test seam — inject the next ADC reading (overrides the hardware read). */
    void debugSetRaw(uint16_t raw) { _testRaw = raw; _testRawSet = true; _lastReadMs = 0; }
#endif

protected:
    uint16_t readRaw() override;   // POLL_MS-throttled ADC read → resolve()

private:
    uint16_t resolve(uint16_t raw) const;   ///< band-resolve a raw value → index or NO_POSITION
    uint16_t posValAt(uint8_t i) const;     ///< explicit posVals[i], or computed equal-spacing
    bool     isValid(uint8_t i) const;      ///< false when posVals[i] == ANALOG_NC

    PinRef          _pin;
    const uint16_t* _posVals;     ///< caller array, or nullptr for equal-spacing
    uint16_t        _deadband;
    uint16_t        _cachedIdx;   ///< last resolved index (held between throttled reads)
    uint32_t        _lastReadMs;
#ifdef ANALOGMULTIPOS_TEST
    uint16_t        _testRaw    = 0;
    bool            _testRawSet = false;
#endif
};

}  // namespace OpenSkyhawk

#endif  // ARDUINO_ARCH_STM32
