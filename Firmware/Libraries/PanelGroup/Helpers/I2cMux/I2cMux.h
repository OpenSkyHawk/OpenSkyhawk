/**
 * @file I2cMux.h
 * @brief TCA9548A 1-to-8 I2C multiplexer channel selector for OpenSkyhawk.
 *
 * @details One I2cMux instance == one TCA9548A chip on one I2C bus. A DrumDisplay
 * (or any future muxed I2C output) holds a pointer to an I2cMux plus a channel index
 * and calls select() before every I2C transaction so the right downstream branch is
 * live. The mux is a passive switch — the U8G2 driver still owns the device address;
 * the mux only routes SDA/SCL.
 *
 * @version 0.1.0
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <Arduino.h>
#include <Wire.h>

namespace OpenSkyhawk {

/**
 * @brief Selects one downstream channel of a TCA9548A I2C multiplexer.
 *
 * @details Stateless beyond a last-selected cache: select(ch) writes the 1-of-8
 * channel bitmask to the TCA9548A control register only when the requested channel
 * differs from the last one written, so repeated select() of the same channel costs
 * no I2C. Construct one per physical TCA9548A. The sketch owns Wire.begin();
 * I2cMux never starts the bus and performs no I2C in its constructor.
 */
class I2cMux {
public:
    /**
     * @brief Construct a mux handle. No I2C occurs here.
     * @param addr  TCA9548A 7-bit I2C address (0x70–0x77 via A0/A1/A2). Default 0x70.
     * @param wire  I2C bus the mux sits on. Default Wire (I2C1 on STM32).
     */
    explicit I2cMux(uint8_t addr = 0x70, TwoWire& wire = Wire);

    /**
     * @brief Route the bus to one downstream channel.
     * @param channel  Channel 0–7. Values above 7 are clamped to 7.
     * @return true if the channel is selected (write issued, or already current);
     *         false on I2C NAK.
     * @note Writes a single byte (1 << channel); skipped when channel == last selected.
     *       Callers sharing one mux across several devices MUST call this immediately
     *       before each downstream I2C op — an interleaved driver can change the channel.
     */
    bool select(uint8_t channel);

    /** @brief Disable all channels (control byte 0x00). Optional bus quiescing. */
    void disableAll();

private:
    uint8_t  _addr;         // TCA9548A 7-bit address
    TwoWire* _wire;         // bus the mux is on
    int8_t   _lastChannel;  // -1 = nothing selected yet; cache to skip redundant writes
};

}  // namespace OpenSkyhawk

#endif  // ARDUINO_ARCH_STM32
