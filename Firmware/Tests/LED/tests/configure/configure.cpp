// LED — configure() test
//
// Verifies:
//   reverse=false: configure() drives pin LOW (LED off).
//   reverse=true:  configure() drives pin HIGH (LED off in sink wiring).
//   configureAsOutput() is called: pin is readable via digitalRead() after configure().
//
// Hardware: STM32. PB0 (reverse=false LED) and PB1 (reverse=true LED).
// No external connections needed — pins are driven as outputs and read back
// via digitalRead() which reads the GPIO output register, not an external signal.

#include <Arduino.h>
#include <STM32Board.h>
#include <Outputs/LED.h>

static constexpr uint8_t PIN_SOURCE = PB0;  // reverse=false LED
static constexpr uint8_t PIN_SINK   = PB1;  // reverse=true  LED

// Global declarations — self-register into OutputBase list at startup.
OpenSkyhawk::LED gLedSource(0x1111, 0xFFFF, PinRef(PIN_SOURCE));             // default reverse=false
OpenSkyhawk::LED gLedSink  (0x2222, 0xFFFF, PinRef(PIN_SINK),  /*reverse=*/true);

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== LED configure ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    // Call configure() — sets OUTPUT mode and drives off-state.
    gLedSource.configure();
    gLedSink.configure();

    // reverse=false: off state is LOW (current-source — sink pulls low to turn on)
    check("reverse=false: pin LOW after configure()", digitalRead(PIN_SOURCE) == LOW);

    // reverse=true: off state is HIGH (current-sink — pin HIGH keeps LED cathode above GND)
    check("reverse=true:  pin HIGH after configure()", digitalRead(PIN_SINK) == HIGH);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
