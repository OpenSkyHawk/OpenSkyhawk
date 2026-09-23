/**
 * @file Dimmer.h
 * @brief PWM backlight / dimmer output for OpenSkyhawk PanelGroup nodes.
 *
 * @version 0.1.0
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <Outputs/AnalogOutput/AnalogOutput.h>  // family base (OutputBase, PinRef via PanelGroup.h)

namespace OpenSkyhawk {

/**
 * @brief PWM dimmer output — the DcsBios::Dimmer equivalent. Maps a 16-bit DCS-BIOS value to
 *        PWM duty on a direct GPIO timer pin.
 *
 * @details Primary use is backlight zone dimming: the low-side MOSFET gate of an LED zone,
 * one Dimmer per zone (PA6 / PA7 on the PanelGroup base). The A-4E-C drives five such values —
 * LIGHTS_CONSOLE, LIGHTS_INSTRUMENTS, LIGHTS_FLOOD_RED, LIGHTS_FLOOD_WHITE, APG53A_GLOW — and
 * any other PWM-proportional load works the same way.
 *
 * The AnalogOutput base owns matching, decoding and change dedup; this class is the sink:
 * apply() writes through PinRef::writeAnalog() (16-bit in; the GPIO path outputs value >> 8 as
 * 8-bit duty). An optional scale function reshapes the value first — perceptual dimming curves,
 * inversion.
 *
 * Takes a PinRef like every control class and checks its type: brightness comes from an STM32
 * timer channel, so configure() requires a direct GPIO that has one. Anything else (MCP23017,
 * ShiftBus, a GPIO with no timer) is logged and leaves the output disabled rather than degrading
 * silently — writeAnalog() is a no-op through an expander, and analogWrite() on a non-timer GPIO
 * collapses to an on/off threshold. Software PWM through an expander is deliberately not offered:
 * those bits change once per loop or bus transfer, so a duty cycle there would flicker and keep
 * the bus busy, where timer PWM costs no CPU.
 *
 * On/off threshold behaviour is LED's job, not this class's. Polarity is not invertible — the
 * zone-switch hardware standard is fixed (higher duty = brighter); an inverted drive is a scale
 * function, not a constructor flag.
 *
 * Duty is driven to 0 during configure() and stays there (zone dark) until a CTRL_BCAST packet
 * with a matching controlId arrives.
 */
class Dimmer : public AnalogOutput {
public:
    /** @brief Scale function: raw 16-bit DCS value → 16-bit value handed to writeAnalog(). */
    using ScaleFn = uint16_t (*)(uint16_t value);

    /**
     * @brief Construct and register a dimmer.
     * @param controlId  DCS-BIOS output address (A_4E_C_* from A4EC_OutputIds.h — the full
     *                   16-bit word; the matching *_AM mask is 0xffff for these outputs).
     * @param pin        PinRef for the PWM output: a direct STM32 GPIO on a timer channel
     *                   (the zone-switch MOSFET gate) — PinRef(PA6) / PinRef(PA7) on the base.
     * @param scale      Optional reshaping of the 16-bit value before it is written.
     *                   nullptr (default) = identity.
     */
    Dimmer(uint16_t controlId, PinRef pin, ScaleFn scale = nullptr);

    /**
     * @brief Configure the output pin and drive duty to 0.
     *
     * Called by PanelGroup::setup() after chip.init(). Requires a direct GPIO on a timer
     * channel; otherwise logs on DiagSerial and disables the output (no writes at all).
     * On success the pin becomes an output at duty 0, so the zone stays dark until the first
     * matching CTRL_BCAST.
     */
    void configure() override;

#ifdef DIMMER_TEST
    /** @brief Test seam — count of actual writeAnalog() calls (dedup-suppressed ones excluded). */
    uint16_t writeCount() const { return _writeCount; }
    /** @brief Test seam — last 8-bit duty written (0 before the first write). */
    uint8_t lastDuty() const { return _lastDuty; }
    /** @brief Test seam — false when configure() rejected the pin. */
    bool enabled() const { return _enabled; }
#endif

protected:
    /**
     * @brief Write the value as PWM duty. Called by the base when the decoded value changed.
     *
     * Applies the scale function, then skips the write when the resulting 8-bit duty equals
     * the last one written — different 16-bit values map to the same duty. No-op while the
     * output is disabled.
     */
    void apply(uint16_t value) override;

private:
    PinRef   _pin;
    ScaleFn  _scale;                ///< nullptr = identity
    bool     _enabled  = false;     ///< false until configure() accepts the pin
    uint8_t  _lastDuty = 0;         ///< last duty written — dedup reference
    bool     _hasState = false;     ///< _lastDuty valid
#ifdef DIMMER_TEST
    uint16_t _writeCount = 0;       ///< test seam: actual writes (post-dedup)
#endif
};

} // namespace OpenSkyhawk

#endif // ARDUINO_ARCH_STM32
