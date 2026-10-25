/**
 * @file AnalogInput.h
 * @brief Continuous analog input (potentiometer / axis) for OpenSkyhawk PanelGroup nodes.
 *
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <PanelGroup.h>  // InputBase, PinRef

namespace OpenSkyhawk {

/**
 * @brief Tuning for an AnalogInput. Every field has a default, so a sketch sets only what differs,
 *        by name: `{ .hysteresis = 32, .ewmaShift = 4, .pollMs = 2 }` (#295).
 *
 * Fields are only ever appended, each with a default, so a new setting never breaks a sketch.
 * The constants are the defaults and limits the fields refer to.
 */
struct AnalogConfig {
    /// `hysteresis` default: chosen from the controlId's route when the input is constructed —
    /// DCS_HYSTERESIS for a DCSIN_* (DCS-BIOS) id, HID_HYSTERESIS for a CTRL_* (HID) id. Any other
    /// value is used as given.
    static constexpr uint16_t HYSTERESIS_AUTO    = 0xFFFF;
    /// DCS-BIOS route: 1024 keeps a sweep from flooding DCS-BIOS import (SoakRig and E2E use it).
    static constexpr uint16_t DCS_HYSTERESIS     = 1024;
    /// HID route: 128 — the value the axis calibration was validated with (#46/#251).
    static constexpr uint16_t HID_HYSTERESIS     = 128;
    static constexpr uint8_t  DEFAULT_EWMA_SHIFT = 3;    ///< EWMA α = 1/2^3 = 1/8.
    static constexpr uint8_t  MAX_EWMA_SHIFT     = 15;   ///< cap: scaled<<shift must fit int32 at full scale.
    static constexpr uint16_t DEFAULT_POLL_MS    = 8;    ///< default min interval between ADC reads (ms).

    /// false: minRaw → 0, maxRaw → 65535. true: inverted.
    bool     reverse    = false;
    /// Raw ADC value mapping to 0. Readings below are clamped.
    uint16_t minRaw     = 0;
    /// Raw ADC value mapping to 65535. Readings above are clamped.
    uint16_t maxRaw     = 65535;
    /// Output counts of movement required before a new value is emitted (HYSTERESIS_AUTO: by route).
    uint16_t hysteresis = HYSTERESIS_AUTO;
    /// EWMA smoothing strength: α = 1/2^ewmaShift. Capped to MAX_EWMA_SHIFT — beyond that the int32
    /// accumulator (scaled << shift) would overflow at full scale.
    uint8_t  ewmaShift  = DEFAULT_EWMA_SHIFT;
    /// Min interval between ADC reads, ms. Per instance, not per node. Not clamped: 0 means read
    /// every loop iteration, which is legal but makes τ track the loop rate — not a controlled
    /// quantity. An ADS1115 PinRef blocks ~8 ms per conversion whatever this says, so keep those at
    /// the default or higher.
    uint16_t pollMs     = DEFAULT_POLL_MS;
};

/**
 * @brief Continuous analog input — one analog `PinRef`, normalised to a 16-bit value 0..65535.
 *        Emits the smoothed value over CAN (MULTIPOS transport). Self-registers into PanelGroup's
 *        InputBase list.
 *
 * @details A **linear** input, not a selector. It shares the MULTIPOS wire transport with the
 * selector family only because DCS-BIOS `set_state` has no separate "continuous" dispatch — the
 * 16-bit value is the control *position*, not an index.
 *
 * Read path (ports DcsBios `PotentiometerEWMA`): read the ADC (already 16-bit — STM32 ×16, or
 * ADS1115 0–3.3V → 0–65534), clamp to `[minRaw, maxRaw]`, map to 0..65535 (reverse-aware), then
 * apply an integer EWMA low-pass filter (α = 1/2^`ewmaShift`). A new value is emitted only when the
 * smoothed value moves more than `hysteresis` counts from the last sent value, or when it reaches a
 * rail (0 / 65535) moving toward it — a settled pot is silent, the endpoints always reached.
 *
 * The ADC is re-read at most every `pollMs` — an AnalogConfig field, default `DEFAULT_POLL_MS`
 * (8 ms). It is **per instance, not per node**: two AnalogInputs on one board may read at different
 * rates, and PanelGroup owns no analog timer of its own (it calls `poll()` every iteration and lets
 * each input decide). `forceReport()` samples fresh (bypassing the throttle) and emits the current
 * value as the baseline. Integer EWMA (a shift, not a divide) keeps it cheap on the FPU-less
 * STM32F103.
 *
 * `pollMs` and `ewmaShift` are **coupled**: the filter time constant is τ ≈ 2^`ewmaShift` ×
 * `pollMs` — the defaults give 2^3 × 8 = 64 ms. Halving `pollMs` halves τ unless `ewmaShift` rises
 * to match, so the two are chosen together, never one alone.
 *
 * configure() does not enable internal pull-ups; the wiper drives the pin directly.
 *
 * Used by: AN/ARC-51A VOL (volume potentiometer).
 */
class AnalogInput : public InputBase {
public:
    /**
     * @brief Construct a continuous analog input.
     *
     * @param controlId  DCSIN_* or CTRL_* constant. Determines PanelBridge routing — and, with
     *                   the default config, the hysteresis (AnalogConfig::HYSTERESIS_AUTO).
     * @param pin        analog PinRef (STM32 ADC GPIO or ADS1115 channel).
     * @param cfg        tuning; set only the fields that differ, e.g. `{ .reverse = true }`.
     */
    AnalogInput(uint16_t controlId, PinRef pin, const AnalogConfig& cfg = {});

    /** @brief Throttled ADC read + EWMA; emit when the value clears the hysteresis or a rail. */
    void poll() override;

    /** @brief Sample fresh (bypassing the throttle) and emit the current value as the baseline. */
    void forceReport() override;

    /** @brief Configure the pin as an input. Called by PanelGroup::setup(). */
    void configure() override;

#ifdef ANALOGINPUT_TEST
    /** @brief Test seam — inject the next raw ADC reading (overrides the hardware read). */
    void debugSetRaw(uint16_t raw) { _testRaw = raw; _testRawSet = true; }
    /** @brief Test seam — run one read + EWMA step, bypassing the pollMs throttle. */
    void debugStep() { sample(); }
    /** @brief Test seam — last emitted 16-bit value. */
    uint16_t value() const { return _lastSent; }
    /** @brief Test seam — current EWMA-smoothed value (pre-hysteresis). */
    uint16_t smoothed() const { return _smoothed; }
    /** @brief Test seam — count of CAN EVTs emitted. */
    uint16_t emitCount() const { return _emitCount; }
    /**
     * @brief Test seam — ADC reads taken. Not the same as emitCount(): hysteresis gates emission,
     *        so a settled input reads at the full rate and emits nothing. This is what observes
     *        the pollMs throttle, which debugStep() bypasses by design.
     */
    uint32_t readCount() const { return _readCount; }
#endif

protected:
    /**
     * @brief Read the raw 16-bit source value. The one hook a family member overrides.
     *
     * Default: the analog PinRef (or the injected value under ANALOGINPUT_TEST). A subclass that
     * reinterprets the reading overrides this and calls AnalogInput::readRaw() for the source
     * value — AngleSensorInput re-centres it on its centre angle. Everything after this point
     * (clamp, scale, EWMA, hysteresis, emit) is the base's and is shared unchanged.
     */
    virtual uint16_t readRaw();

private:
    void     sample();                       ///< one read + EWMA step + conditional emit (no throttle).
    uint16_t readScaled();                   ///< readRaw(), clamp [min,max], map → 0..65535.
    bool     shouldEmit(uint16_t v) const;   ///< hysteresis + near-rail test vs _lastSent.
    void     emit(uint16_t v, bool init = false);

    uint16_t _controlId;
    PinRef   _pin;
    bool     _reverse;
    uint16_t _minRaw;
    uint16_t _maxRaw;
    uint16_t _hysteresis;
    uint8_t  _ewmaShift;
    uint16_t _pollMs;       ///< min ms between ADC reads; 0 = read every loop iteration.
    int32_t  _acc;          ///< EWMA accumulator (smoothed value << ewmaShift).
    uint16_t _smoothed;     ///< current EWMA output.
    uint16_t _lastSent;     ///< last emitted value.
    uint32_t _lastReadMs;
    bool     _initialized;  ///< false until forceReport(); poll() no-op before this.
#ifdef ANALOGINPUT_TEST
    uint16_t _emitCount  = 0;
    uint32_t _readCount  = 0;
    uint16_t _testRaw    = 0;
    bool     _testRawSet = false;
#endif
};

}  // namespace OpenSkyhawk

#endif  // ARDUINO_ARCH_STM32
