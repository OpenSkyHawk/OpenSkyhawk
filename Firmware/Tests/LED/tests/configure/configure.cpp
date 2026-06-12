// LED — configure() test
//
// Verifies:
//   reverse=false: configure() drives pin LOW (LED off).
//   reverse=true:  configure() drives pin HIGH (LED off in sink wiring).
//   configureAsOutput() is called: pin is readable via digitalRead() after configure().
//
// Hardware: STM32. GPIO only. No CAN, no MCP23017.
// PB0 and PB1 are safe GPIO pins (not used by CAN, I2C, SPI, or USART1).

#include <Arduino.h>
#include <LED.h>

static constexpr uint8_t PIN_SOURCE = PB0;  // reverse=false LED
static constexpr uint8_t PIN_SINK   = PB1;  // reverse=true  LED

// Global declarations — self-register into OutputBase list at startup.
OpenSkyhawk::LED gLedSource(0x1111, 0xFFFF, PinRef(PIN_SOURCE));             // default reverse=false
OpenSkyhawk::LED gLedSink  (0x2222, 0xFFFF, PinRef(PIN_SINK),  /*reverse=*/true);

void setup() {
    Serial.begin(115200);
    while (!Serial) {}
    Serial.println("=== LED configure ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        Serial.print(label);
        Serial.println(ok ? ": PASS" : ": FAIL");
    };

    // Call configure() — sets OUTPUT mode and drives off-state.
    gLedSource.configure();
    gLedSink.configure();

    // reverse=false: off state is LOW (current-source — sink pulls low to turn on)
    check("reverse=false: pin LOW after configure()", digitalRead(PIN_SOURCE) == LOW);

    // reverse=true: off state is HIGH (current-sink — pin HIGH keeps LED cathode above GND)
    check("reverse=true:  pin HIGH after configure()", digitalRead(PIN_SINK) == HIGH);

    Serial.println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
