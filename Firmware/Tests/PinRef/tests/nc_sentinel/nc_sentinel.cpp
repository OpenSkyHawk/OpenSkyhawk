// PinRef — NC sentinel test
//
// Verifies: PIN_NC.read() == false, readAnalog() == 0, write() and writeAnalog()
// compile and are no-ops (no crash), isGpio() == false, isNC() == true.
//
// No hardware wiring required — exercises default constructor only.

#include <Arduino.h>
#include <STM32Board.h>
#include <PinRef.h>

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== PinRef nc_sentinel ===");

    // Default constructor produces NC sentinel
    PinRef nc;

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    check("PIN_NC.read()      == false", PIN_NC.read()      == false);
    check("PIN_NC.readAnalog()== 0    ", PIN_NC.readAnalog()== 0);
    check("PIN_NC.isGpio()    == false", PIN_NC.isGpio()    == false);
    check("PIN_NC.isNC()      == true ", PIN_NC.isNC()      == true);

    // PinRef() default ctor is identical to PIN_NC
    check("PinRef().isNC()    == true ", nc.isNC()          == true);

    // write and writeAnalog must compile and not crash
    nc.write(true);
    nc.write(false);
    nc.writeAnalog(0);
    nc.writeAnalog(65535);
    STM32Board::diagSerial().println("write/writeAnalog: compiled and ran (no crash)");

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
