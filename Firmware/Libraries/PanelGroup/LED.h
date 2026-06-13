/**
 * @file LED.h
 * @brief Digital LED output for OpenSkyhawk PanelGroup nodes.
 *
 * @version 0.1.0
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <PanelGroup.h>  // OutputBase, PinRef

namespace OpenSkyhawk {

/**
 * @brief Digital LED output. Drives a pin based on a DCS-BIOS state value.
 *
 * @details Receives state via onControlPacket() — called by PanelGroup when a
 * CTRL_BCAST frame arrives. Ignores packets whose controlId does not match.
 *
 * The mask parameter handles DCS-BIOS bit-packed outputs, where a single 16-bit
 * address carries multiple independent flags. For whole-word binary outputs
 * (value is 0 or 1), use mask = 0xFFFF.
 *
 * The reverse parameter handles LEDs wired with current-sinking polarity —
 * for example, an indicator LED with its anode tied to VCC through a resistor,
 * where the MCU or MCP23017 output sinks current (LOW = on).
 * reverse = false (default): (value & mask) != 0 → pin HIGH (on).
 * reverse = true:            (value & mask) != 0 → pin LOW  (on).
 *
 * Pin is driven to the off state during configure() and remains off until a
 * CTRL_BCAST packet with a matching controlId is received.
 *
 * Works with GPIO and MCP23017 PinRefs. ADS1115 PinRefs are input-only —
 * do not assign one to an LED.
 */
class LED : public OutputBase {
public:
    /**
     * @brief Construct and register an LED output.
     * @param controlId  DCS-BIOS output address (A_4E_C_* from A4EC_OutputIds.h).
     * @param mask       Bitmask: (value & mask) != 0 → on; == 0 → off.
     *                   Use A_4E_C_*_AM constants, or 0xFFFF for whole-word outputs.
     * @param pin        GPIO or MCP23017 PinRef for the LED pin.
     * @param reverse    false (default): pin HIGH = on (current-source wiring).
     *                   true: pin LOW = on (current-sink, anode to VCC).
     */
    LED(uint16_t controlId, uint16_t mask, PinRef pin, bool reverse = false);

    /**
     * @brief Configure pin as output and drive it to the off state.
     *
     * Called by PanelGroup::setup() after chip.init(). Sets OUTPUT mode on GPIO
     * pins; sets IODIR=0 and GPPU=0 on MCP23017 pins. Drives off immediately:
     * LOW when reverse = false, HIGH when reverse = true.
     */
    void configure() override;

    /**
     * @brief Update LED state from a CTRL_BCAST ControlPacket.
     *
     * @param controlId  Incoming packet controlId. Ignored if != _controlId.
     * @param value      Raw 16-bit DCS-BIOS value.
     */
    void onControlPacket(uint16_t controlId, uint16_t value) override;

private:
    uint16_t _controlId;
    uint16_t _mask;
    PinRef   _pin;
    bool     _reverse;   ///< true = current-sink wiring (LOW = on)
    bool     _lastOn   = false;
    bool     _hasState = false;  ///< false until first matching CTRL_BCAST received
};

} // namespace OpenSkyhawk

#endif // ARDUINO_ARCH_STM32
