/**
 * @file AnalogOutput.h
 * @brief Family base for outputs driven by one 16-bit DCS-BIOS value.
 *
 * @version 0.1.0
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <PanelGroup.h>  // OutputBase

namespace OpenSkyhawk {

/**
 * @brief Abstract base for outputs driven by one 16-bit DCS-BIOS value (FirmwarePlan D16).
 *
 * @details Owns what every member shares, so a subclass is only its sink:
 * controlId matching on each CTRL_BCAST packet, (value & mask) >> shift decoding — the same
 * address/mask/shift triple DCS-BIOS uses, so packed fields work as well as whole-word
 * outputs — and change dedup, so apply() runs only when the decoded value differs from the
 * last one applied. DCS-BIOS re-broadcasts full state after SYNC_REQ; without the dedup every
 * subclass would repeat the same work on every resync.
 *
 * Does not know what the value means or where it goes — that is the subclass's apply().
 * Does not configure pins: a subclass with hardware overrides configure().
 *
 * Members: Dimmer (PWM duty on a GPIO timer pin), IntegerOutput (user callback).
 * Needle gauges are deliberately not members — they use NeedleGauge + a MotorDriver, which
 * add calibration and smooth motion.
 */
class AnalogOutput : public OutputBase {
public:
    /**
     * @brief Match, decode, dedup, then apply(). Called by PanelGroup for every CTRL_BCAST packet.
     *
     * @param controlId  Incoming packet controlId. Ignored if != the configured one.
     * @param value      Raw 16-bit DCS-BIOS value.
     */
    void onControlPacket(uint16_t controlId, uint16_t value) final;

protected:
    /**
     * @brief Construct the base. Registration into PanelGroup's output list is OutputBase's.
     * @param controlId  DCS-BIOS output address (A_4E_C_* from A4EC_OutputIds.h).
     * @param mask       Bits of the 16-bit word belonging to this output. Default: all.
     *                   Use the A_4E_C_*_AM constants for packed fields.
     * @param shift      Right-shift applied after masking. Default 0.
     */
    AnalogOutput(uint16_t controlId, uint16_t mask = 0xFFFF, uint8_t shift = 0);

    /**
     * @brief Sink hook — called with the decoded value, only when it changed.
     * @param value  (raw & mask) >> shift.
     */
    virtual void apply(uint16_t value) = 0;

private:
    uint16_t _controlId;
    uint16_t _mask;
    uint8_t  _shift;
    uint16_t _lastValue = 0;      ///< last decoded value applied — dedup reference
    bool     _hasValue  = false;  ///< false until the first matching packet
};

} // namespace OpenSkyhawk

#endif // ARDUINO_ARCH_STM32
