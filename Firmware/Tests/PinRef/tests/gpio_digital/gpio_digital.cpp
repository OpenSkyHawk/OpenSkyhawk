// PinRef — GPIO digital read/write test
//
// Hardware: loopback wire PB0 → PA8, PB1 → PA9.
// Drives output PinRef HIGH/LOW and reads back via input PinRef on the loopback pair.
// Verifies that read() returns true when driven HIGH and false when driven LOW.

#include <Arduino.h>
#include <STM32Board.h>
#include <PinRef.h>

static PinRef out0(PB0);
static PinRef out1(PB1);
static PinRef in0(PA8);
static PinRef in1(PA9);

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== PinRef gpio_digital ===");
    STM32Board::diagSerial().println("Hardware: PB0->PA8, PB1->PA9 loopback wires required.");

    out0.configureAsOutput();
    out1.configureAsOutput();
    in0.configureAsInput();
    in1.configureAsInput();

    bool pass = true;
    auto check = [&](const char* label, bool expected, bool actual) {
        bool ok = (actual == expected);
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    // Drive HIGH — read back HIGH
    out0.write(true);
    out1.write(true);
    delayMicroseconds(10);
    check("PB0->PA8 HIGH", true,  in0.read());
    check("PB1->PA9 HIGH", true,  in1.read());

    // Drive LOW — read back LOW
    out0.write(false);
    out1.write(false);
    delayMicroseconds(10);
    check("PB0->PA8 LOW ", false, in0.read());
    check("PB1->PA9 LOW ", false, in1.read());

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
