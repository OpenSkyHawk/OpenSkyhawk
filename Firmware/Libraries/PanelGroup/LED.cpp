#ifdef ARDUINO_ARCH_STM32

#include "LED.h"

OpenSkyhawk::LED::LED(uint16_t controlId, uint16_t mask, PinRef pin, bool reverse)
    : _controlId(controlId), _mask(mask), _pin(pin), _reverse(reverse) {}

void OpenSkyhawk::LED::configure() {
    _pin.configureAsOutput();
    _pin.write(_reverse); // reverse=false → LOW (off); reverse=true → HIGH (off)
}

void OpenSkyhawk::LED::onControlPacket(uint16_t controlId, uint16_t value) {
    if (controlId != _controlId) return;
    bool on = (value & _mask) != 0;
    _pin.write(_reverse ? !on : on);
}

#endif // ARDUINO_ARCH_STM32
