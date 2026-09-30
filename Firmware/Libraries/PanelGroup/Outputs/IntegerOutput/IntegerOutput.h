/**
 * @file IntegerOutput.h
 * @brief Callback output for OpenSkyhawk PanelGroup nodes — the escape hatch.
 *
 * @version 0.1.0
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <Outputs/AnalogOutput/AnalogOutput.h>  // family base

namespace OpenSkyhawk {

/**
 * @brief Callback output — the DcsBios::IntegerBuffer equivalent, for outputs no built-in class
 *        covers (a custom display, an LCD, a bespoke actuator).
 *
 * @details Calls a user-supplied callback with the decoded value whenever it changes. No PinRef:
 * the callback owns whatever hardware is involved.
 *
 * Matching, (value & mask) >> shift decoding and change dedup come from the AnalogOutput base,
 * so the callback runs once per change rather than once per CTRL_BCAST. It runs in
 * PanelGroup::loop() context — never from an ISR — and must not block: a slow callback delays
 * every other output and the next CAN drain.
 */
class IntegerOutput : public AnalogOutput {
public:
    /** @brief Callback type: receives the decoded value. */
    using Callback = void (*)(uint16_t value);

    /**
     * @brief Construct and register a callback output.
     * @param controlId  DCS-BIOS output address (A_4E_C_* from A4EC_OutputIds.h).
     * @param callback   Called with the decoded value on every change. nullptr is accepted
     *                   (the output then does nothing) so a sketch can stub one out.
     * @param mask       Bits of the 16-bit word belonging to this output. Default: all.
     * @param shift      Right-shift applied after masking. Default 0.
     */
    IntegerOutput(uint16_t controlId, Callback callback, uint16_t mask = 0xFFFF, uint8_t shift = 0);

protected:
    /** @brief Invoke the callback with the decoded value. Called by the base on a change. */
    void apply(uint16_t value) override;

private:
    Callback _callback;
};

} // namespace OpenSkyhawk

#endif // ARDUINO_ARCH_STM32
