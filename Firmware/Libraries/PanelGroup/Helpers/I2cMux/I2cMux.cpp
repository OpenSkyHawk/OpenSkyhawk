/**
 * @file I2cMux.cpp
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#ifdef ARDUINO_ARCH_STM32

#include "I2cMux.h"

namespace OpenSkyhawk {

I2cMux::I2cMux(uint8_t addr, TwoWire& wire)
    : _addr(addr), _wire(&wire), _lastChannel(-1) {}

bool I2cMux::select(uint8_t channel) {
    if (channel > 7) channel = 7;
    if (static_cast<int8_t>(channel) == _lastChannel) return true;  // skip redundant write
    _wire->beginTransmission(_addr);
    _wire->write(static_cast<uint8_t>(1u << channel));
    bool ok = (_wire->endTransmission() == 0);
    if (ok) _lastChannel = static_cast<int8_t>(channel);
    return ok;
}

void I2cMux::disableAll() {
    _wire->beginTransmission(_addr);
    _wire->write(static_cast<uint8_t>(0x00));
    _wire->endTransmission();
    _lastChannel = -1;
}

}  // namespace OpenSkyhawk

#endif  // ARDUINO_ARCH_STM32
