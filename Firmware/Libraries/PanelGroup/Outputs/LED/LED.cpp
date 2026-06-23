#ifdef ARDUINO_ARCH_STM32

#include "LED.h"
#include <STM32Board.h>

OpenSkyhawk::LED::LED(uint16_t controlId, uint16_t mask, PinRef pin, bool reverse)
    : _controlId(controlId), _mask(mask), _pin(pin), _reverse(reverse) {}

void OpenSkyhawk::LED::configure() {
    _pin.configureAsOutput();
    _pin.write(_reverse); // reverse=false → LOW (off); reverse=true → HIGH (off)
}

void OpenSkyhawk::LED::onControlPacket(uint16_t controlId, uint16_t value) {
    if (controlId != _controlId) return;
    bool on = (value & _mask) != 0;
    if (_hasState && on == _lastOn) return;
    _lastOn   = on;
    _hasState = true;
    _pin.write(_reverse ? !on : on);
#ifdef LED_TEST
    _writeCount++;
#endif
    if (STM32Board::isDebug()) {
        auto& d = STM32Board::diagSerial();
        d.print(F("[LED] 0x")); d.print(_controlId, HEX);
        d.println(on ? F(": ON") : F(": OFF"));
    }
}

#endif // ARDUINO_ARCH_STM32
