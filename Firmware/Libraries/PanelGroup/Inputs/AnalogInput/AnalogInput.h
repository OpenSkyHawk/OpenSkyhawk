/**
 * @file AnalogInput.h
 * @brief Continuous analog input (potentiometer / axis) for OpenSkyhawk PanelGroup nodes.
 *
 * @version 0.1.0
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <PanelGroup.h>  // InputBase, PinRef

namespace OpenSkyhawk {

/**
 * @brief Continuous analog input — one analog `PinRef`, normalised to a 16-bit value 0..65535.
 *        Emits the smoothed value over CAN (MULTIPOS transport). Self-registers into PanelGroup's
 *        InputBase list.
 *
 * @details A **linear** input, not a selector. It shares the MULTIPOS wire transport with the
 * selector family only because DCS-BIOS `set_state` has no separate "continuous" dispatch — the
 * 16-bit value is the control *position*, not an index.
 *
 * Read path (ports DcsBios `PotentiometerEWMA`): read the ADC (already 16-bit — STM32 ×16 or
 * ADS1115 ×2), clamp to `[minRaw, maxRaw]`, map to 0..65535 (reverse-aware), then apply an integer
 * EWMA low-pass filter (α = 1/2^`ewmaShift`). A new value is emitted only when the smoothed value
 * moves more than `hysteresis` counts from the last sent value, or when it reaches a rail (0 /
 * 65535) moving toward it — so a settled pot is silent and the endpoints are always reached.
 *
 * The ADC is re-read at most every `POLL_MS` (8 ms); `forceReport()` samples fresh (bypassing the
 * throttle) and emits the current value as the baseline. Integer EWMA (a shift, not a divide)
 * keeps it cheap on the FPU-less STM32F103.
 *
 * configure() does not enable internal pull-ups; the wiper drives the pin directly.
 *
 * Used by: AN/ARC-51A VOL (volume potentiometer).
 */
class AnalogInput : public InputBase {
public:
    static constexpr uint16_t DEFAULT_HYSTERESIS = 128;  ///< counts on the 16-bit output.
    static constexpr uint8_t  DEFAULT_EWMA_SHIFT = 3;    ///< EWMA α = 1/2^3 = 1/8.
    static constexpr uint16_t POLL_MS            = 8;    ///< min interval between ADC reads (ms).

    /**
     * @brief Construct a continuous analog input.
     *
     * @param controlId   DCSIN_* or CTRL_* constant. Determines PanelBridge routing.
     * @param pin         analog PinRef (STM32 ADC GPIO or ADS1115 channel).
     * @param reverse     false (default): minRaw → 0, maxRaw → 65535. true: inverted.
     * @param minRaw      raw ADC value mapping to 0 (default 0). Readings below are clamped.
     * @param maxRaw      raw ADC value mapping to 65535 (default 65535). Above are clamped.
     * @param hysteresis  output counts of movement required before a new value is emitted.
     * @param ewmaShift   EWMA smoothing strength: α = 1/2^ewmaShift (default 3 → 1/8).
     */
    AnalogInput(uint16_t controlId, PinRef pin, bool reverse = false,
                uint16_t minRaw = 0, uint16_t maxRaw = 65535,
                uint16_t hysteresis = DEFAULT_HYSTERESIS, uint8_t ewmaShift = DEFAULT_EWMA_SHIFT);

    /** @brief Throttled ADC read + EWMA; emit when the value clears the hysteresis or a rail. */
    void poll() override;

    /** @brief Sample fresh (bypassing the throttle) and emit the current value as the baseline. */
    void forceReport() override;

    /** @brief Configure the pin as an input. Called by PanelGroup::setup(). */
    void configure() override;

#ifdef ANALOGINPUT_TEST
    /** @brief Test seam — inject the next raw ADC reading (overrides the hardware read). */
    void debugSetRaw(uint16_t raw) { _testRaw = raw; _testRawSet = true; }
    /** @brief Test seam — run one read + EWMA step, bypassing the 8 ms throttle. */
    void debugStep() { sample(); }
    /** @brief Test seam — last emitted 16-bit value. */
    uint16_t value() const { return _lastSent; }
    /** @brief Test seam — current EWMA-smoothed value (pre-hysteresis). */
    uint16_t smoothed() const { return _smoothed; }
    /** @brief Test seam — count of CAN EVTs emitted. */
    uint16_t emitCount() const { return _emitCount; }
#endif

private:
    void     sample();                       ///< one read + EWMA step + conditional emit (no throttle).
    uint16_t readScaled();                   ///< read ADC (test seam), clamp [min,max], map → 0..65535.
    bool     shouldEmit(uint16_t v) const;   ///< hysteresis + near-rail test vs _lastSent.
    void     emit(uint16_t v, bool init = false);

    uint16_t _controlId;
    PinRef   _pin;
    bool     _reverse;
    uint16_t _minRaw;
    uint16_t _maxRaw;
    uint16_t _hysteresis;
    uint8_t  _ewmaShift;
    int32_t  _acc;          ///< EWMA accumulator (smoothed value << ewmaShift).
    uint16_t _smoothed;     ///< current EWMA output.
    uint16_t _lastSent;     ///< last emitted value.
    uint32_t _lastReadMs;
    bool     _initialized;  ///< false until forceReport(); poll() no-op before this.
#ifdef ANALOGINPUT_TEST
    uint16_t _emitCount  = 0;
    uint16_t _testRaw    = 0;
    bool     _testRawSet = false;
#endif
};

}  // namespace OpenSkyhawk

#endif  // ARDUINO_ARCH_STM32
