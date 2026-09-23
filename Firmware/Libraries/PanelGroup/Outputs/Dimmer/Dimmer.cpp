#ifdef ARDUINO_ARCH_STM32

#include "Dimmer.h"
#include <STM32Board.h>

OpenSkyhawk::Dimmer::Dimmer(uint16_t controlId, PinRef pin, ScaleFn scale)
    : AnalogOutput(controlId), _pin(pin), _scale(scale) {}

void OpenSkyhawk::Dimmer::configure() {
    // digitalPinHasPWM() is the core's own pin-map lookup (pins_arduino.h → PinMap_TIM), so a
    // GPIO without a timer channel is caught here instead of being discovered as a zone that
    // only switches fully on and off.
    _enabled = _pin.isGpio() && digitalPinHasPWM(_pin.gpioPin());
    if (!_enabled) {
        STM32Board::log("[Dimmer] pin is not a timer-capable GPIO — output disabled");
        return;                        // never drive a pin that cannot dim
    }
    _pin.configureAsOutput();
    _pin.writeAnalog(0);               // zone dark until the first CTRL_BCAST
    _lastDuty = 0;
    _hasState = true;
}

void OpenSkyhawk::Dimmer::apply(uint16_t value) {
    if (!_enabled) return;
    const uint16_t v    = _scale ? _scale(value) : value;
    const uint8_t  duty = (uint8_t)(v >> 8);      // what the GPIO path actually outputs
    // analogWrite() reconfigures the timer channel on every call, so skip the ones that would
    // land on the duty already being driven.
    if (_hasState && duty == _lastDuty) return;
    _pin.writeAnalog(v);
    _lastDuty = duty;
    _hasState = true;
#ifdef DIMMER_TEST
    _writeCount++;
#endif
    if (STM32Board::isDebug()) {
        auto& d = STM32Board::diagSerial();
        d.print(F("[DIM] 0x")); d.print(value, HEX);
        d.print(F(" duty=")); d.println(duty);
    }
}

#endif // ARDUINO_ARCH_STM32
