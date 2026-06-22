/**
 * @file SwitchMultiPos.h
 * @brief N-pin rotary selector switch for OpenSkyhawk PanelGroup nodes.
 *
 * @version 0.1.0
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <PanelGroup.h>                          // PinRef
#include <Inputs/MultiPosInput/MultiPosInput.h>  // MultiPosInput base

namespace OpenSkyhawk {

/**
 * @brief Multi-position rotary selector — N discrete pins, exactly one active at a time.
 *        Emits the active position index 0..N-1 over CAN (MULTIPOS dispatch).
 *
 * @details Self-registers into PanelGroup's InputBase list (via MultiPosInput).
 *
 * One-hot read (reverse = false, default): the active pin reads LOW (closed to GND); its
 * array index is the position. reverse = true inverts it (active pin reads HIGH).
 *
 * If no pin reads active — e.g. a non-shorting rotary mid-throw — the last confirmed
 * position is held; no spurious EVT. A `PIN_NC` entry marks a mechanical-only detent (a
 * position with no physical pin, such as a sprung OFF): when no electrical pin is active,
 * that detent's index is reported.
 *
 * Debounce: 20 ms on the resolved index — a changed position must hold steady for the
 * window before it is confirmed and emitted. Absorbs contact bounce and fast throws
 * (intermediate detents held < 20 ms are skipped; only the settled position emits). The
 * value is an absolute index, so jumping across positions is safe.
 *
 * configure() does not enable internal pull-ups; the schematic supplies input bias
 * (typically 10 kΩ to +3.3V, switch to GND).
 */
class SwitchMultiPos : public MultiPosInput {
public:
    static constexpr uint16_t DEBOUNCE_MS = 20;

    /**
     * @brief Construct an N-position selector.
     *
     * @param controlId  DCSIN_* or CTRL_* constant. Determines PanelBridge routing.
     * @param pins       Pointer to a caller-owned array of N PinRefs, one per position. The
     *                   array must outlive this object (define it static/global, like the
     *                   sketch wiring map). Use PIN_NC for a mechanical-only detent.
     * @param numPins    N — number of entries in @p pins (valid indices 0..N-1).
     * @param reverse    false (default): active pin reads LOW. true: active pin reads HIGH.
     */
    SwitchMultiPos(uint16_t controlId, const PinRef* pins, uint8_t numPins, bool reverse = false);

    /**
     * @brief Configure each non-NC pin as an input. Called by PanelGroup::setup().
     *
     * Does not enable internal pull-ups; board wiring supplies input bias.
     *
     * @note Must not run from the constructor — MCP23017 register writes require the
     * expander to be initialised first.
     */
    void configure() override;

protected:
    /**
     * @brief One-hot scan: return the index of the first active pin, or the PIN_NC detent
     *        index, or NO_POSITION if nothing is active.
     */
    uint16_t readRaw() override;

private:
    const PinRef* _pins;     ///< caller-owned array of N position pins.
    uint8_t       _numPins;  ///< N.
    bool          _reverse;  ///< true = active-HIGH.
};

}  // namespace OpenSkyhawk

#endif  // ARDUINO_ARCH_STM32
