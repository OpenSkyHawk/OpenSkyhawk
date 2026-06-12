// PinRef — GPIO identity test
//
// Verifies: PinRef(PA0).isGpio() == true, gpioPin() returns PA0.
// Verifies: PIN_NC.isGpio() == false.
//
// No hardware wiring required — reads identity metadata only, does not drive pins.

#include <Arduino.h>
#include <STM32Board.h>
#include <PinRef.h>

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== PinRef gpio_identity ===");

    PinRef p(PA0);

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    check("GPIO.isGpio()   == true ", p.isGpio()       == true);
    check("GPIO.gpioPin()  == PA0  ", p.gpioPin()       == PA0);
    check("GPIO.isNC()     == false", p.isNC()          == false);
    check("PIN_NC.isGpio() == false", PIN_NC.isGpio()   == false);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
