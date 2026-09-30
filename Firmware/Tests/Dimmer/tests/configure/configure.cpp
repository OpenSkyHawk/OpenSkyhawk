// Dimmer — configure() test
//
// Verifies:
//   A timer-capable GPIO is accepted: pin drives duty 0 (constant LOW) and the output is enabled.
//   A GPIO with no timer channel is rejected: output disabled, and apply() never writes.
//   An MCP23017 PinRef is rejected the same way (no PWM through an expander).
//
// Hardware: STM32 alone. PB0 (TIM3_CH3) is driven as an output; PA4 has no timer channel and is
// never configured or written. The MCP23017 object is constructed but never talks on the bus —
// configure() only inspects the PinRef type, so no I2C device is needed.

#include <Arduino.h>
#include <Wire.h>
#include <MCP23017.h>
#include <STM32Board.h>
#include <Outputs/Dimmer/Dimmer.h>

static constexpr uint8_t  PIN_PWM    = PB0;   // TIM3_CH3 — timer-capable
static constexpr uint8_t  PIN_NO_TIM = PA4;   // no entry in PinMap_TIM
static constexpr uint16_t CTRL_ID    = 0x1234;

MCP23017 gExpander(0x20, Wire);

// Global declarations — self-register into the OutputBase list at startup.
OpenSkyhawk::Dimmer gGood(CTRL_ID, PinRef(PIN_PWM));
OpenSkyhawk::Dimmer gNoTimer(CTRL_ID, PinRef(PIN_NO_TIM));
OpenSkyhawk::Dimmer gExpanderPin(CTRL_ID, PinRef(gExpander, 0 /*PORT_A*/, 0));

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
    STM32Board::diagSerial().println("=== Dimmer configure ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    gGood.configure();
    gNoTimer.configure();
    gExpanderPin.configure();

    check("timer GPIO: enabled                 ", gGood.enabled());
    check("timer GPIO: duty 0 (pin LOW)        ", constantLow(PIN_PWM));
    check("timer GPIO: no write yet            ", gGood.writeCount() == 0);

    check("GPIO without timer: disabled        ", !gNoTimer.enabled());
    check("MCP23017 pin: disabled              ", !gExpanderPin.enabled());

    // A disabled output must stay inert — full brightness changes nothing.
    gNoTimer.onControlPacket(CTRL_ID, 0xFFFF);
    gExpanderPin.onControlPacket(CTRL_ID, 0xFFFF);
    check("disabled: no write on GPIO-no-timer ", gNoTimer.writeCount() == 0);
    check("disabled: no write on MCP23017 pin  ", gExpanderPin.writeCount() == 0);

    // The accepted one still works after the rejected ones ran.
    gGood.onControlPacket(CTRL_ID, 0xFFFF);
    check("timer GPIO: full value -> pin HIGH  ", mostlyHigh(PIN_PWM));

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
