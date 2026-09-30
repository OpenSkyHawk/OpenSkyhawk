// Dimmer — default value → duty mapping test
//
// Verifies the default map (PinRef::writeAnalog outputs value >> 8 on the GPIO path):
//   0x0000 -> duty 0   — constant LOW, readable with digitalRead()
//   0xFFFF -> duty 255 — constant HIGH, readable with digitalRead()
//   0x8000 -> duty 128 — ~50% PWM, not digital-readable, so it is checked through the test seam
//                        (the real brightness check is the LED-string rig in #209 / #291)
//
// Hardware: STM32 alone. PB0 (TIM3_CH3), nothing wired.

#include <Arduino.h>
#include <STM32Board.h>
#include <Outputs/Dimmer/Dimmer.h>

static constexpr uint8_t  PIN_PWM = PB0;
static constexpr uint16_t CTRL_ID = 0x1234;

OpenSkyhawk::Dimmer gDim(CTRL_ID, PinRef(PIN_PWM));

// A PWM pin at full duty may still glitch low for one count per period on some cores, so sample
// the level instead of trusting a single read. Duty 0 is a true constant LOW.
static bool mostlyHigh(uint8_t pin) {
    uint8_t high = 0;
    for (uint8_t i = 0; i < 32; i++) { if (digitalRead(pin) == HIGH) high++; delayMicroseconds(37); }
    return high >= 28;
}
static bool constantLow(uint8_t pin) {
    for (uint8_t i = 0; i < 32; i++) { if (digitalRead(pin) != LOW) return false; delayMicroseconds(37); }
    return true;
}

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== Dimmer mapping ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    gDim.configure();   // duty 0, zone dark

    // Zero is already the configured duty: the value applies, the redundant write does not.
    gDim.onControlPacket(CTRL_ID, 0x0000);
    check("0x0000 -> pin LOW                 ", constantLow(PIN_PWM));
    check("0x0000 -> duty 0                  ", gDim.lastDuty() == 0);
    check("0x0000 -> redundant write skipped ", gDim.writeCount() == 0);

    gDim.onControlPacket(CTRL_ID, 0xFFFF);
    check("0xFFFF -> duty 255                ", gDim.lastDuty() == 255);
    check("0xFFFF -> pin HIGH                ", mostlyHigh(PIN_PWM));

    gDim.onControlPacket(CTRL_ID, 0x8000);
    check("0x8000 -> duty 128                ", gDim.lastDuty() == 128);
    check("three values -> two writes        ", gDim.writeCount() == 2);

    gDim.onControlPacket(CTRL_ID, 0x0000);
    check("back to 0x0000 -> pin LOW again   ", constantLow(PIN_PWM));

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
