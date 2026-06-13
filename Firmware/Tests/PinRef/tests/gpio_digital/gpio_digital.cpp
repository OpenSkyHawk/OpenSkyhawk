// PinRef — GPIO digital read/write test
//
// Hardware: loopback wire PB0 → PA0, PB1 → PA1.
// Drives output PinRef HIGH/LOW and reads back via input PinRef on the loopback pair.
// Verifies that read() returns true when driven HIGH and false when driven LOW.
//
// PA9/PA10 are USART1 TX/RX (diagSerial) — do NOT use as loopback targets.

#include <Arduino.h>
#include <STM32Board.h>
#include <PinRef.h>

static PinRef out0(PB0);
static PinRef out1(PB1);
static PinRef in0(PA0);
static PinRef in1(PA1);

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== PinRef gpio_digital ===");
    STM32Board::diagSerial().println("Hardware: PB0->PA0, PB1->PA1 loopback wires required.");

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
    check("PB0->PA0 HIGH", true,  in0.read());
    check("PB1->PA1 HIGH", true,  in1.read());

    // Drive LOW — read back LOW
    out0.write(false);
    out1.write(false);
    delayMicroseconds(10);
    check("PB0->PA0 LOW ", false, in0.read());
    check("PB1->PA1 LOW ", false, in1.read());

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
