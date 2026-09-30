#ifdef ARDUINO_ARCH_STM32

#include "IntegerOutput.h"

OpenSkyhawk::IntegerOutput::IntegerOutput(uint16_t controlId, Callback callback,
                                          uint16_t mask, uint8_t shift)
    : AnalogOutput(controlId, mask, shift), _callback(callback) {}

void OpenSkyhawk::IntegerOutput::apply(uint16_t value) {
    if (_callback) _callback(value);
}

#endif // ARDUINO_ARCH_STM32
