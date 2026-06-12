// LED — on/off cycling test (reverse=false)
//
// Verifies:
//   value != 0 → pin HIGH (LED on, current-source wiring).
//   value == 0 → pin LOW  (LED off).
//   Consecutive on/off/on cycles all take effect.
//
// Hardware: STM32. GPIO PB0 only. No CAN, no MCP23017.

#include <Arduino.h>
#include <STM32Board.h>
#include <LED.h>

static constexpr uint8_t TEST_PIN   = PB0;
static constexpr uint16_t CTRL_ID   = 0x1234;
static constexpr uint16_t MASK      = 0xFFFF;

OpenSkyhawk::LED gLed(CTRL_ID, MASK, PinRef(TEST_PIN));

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== LED on_off ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    gLed.configure(); // sets OUTPUT, drives LOW (off)
    check("Initial: pin LOW after configure()", digitalRead(TEST_PIN) == LOW);

    // Turn on
    gLed.onControlPacket(CTRL_ID, 0x0001);
    check("value=1 → pin HIGH (on)", digitalRead(TEST_PIN) == HIGH);

    // Turn off
    gLed.onControlPacket(CTRL_ID, 0x0000);
    check("value=0 → pin LOW (off)", digitalRead(TEST_PIN) == LOW);

    // Turn on again — consecutive cycle works
    gLed.onControlPacket(CTRL_ID, 0xFFFF);
    check("value=0xFFFF → pin HIGH (on)", digitalRead(TEST_PIN) == HIGH);

    // Off again
    gLed.onControlPacket(CTRL_ID, 0x0000);
    check("value=0 again → pin LOW (off)", digitalRead(TEST_PIN) == LOW);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
