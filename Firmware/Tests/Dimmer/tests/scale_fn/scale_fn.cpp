// Dimmer — custom scale function test
//
// Verifies that a ScaleFn reshapes the value before it becomes duty:
//   identity  — same as the default map (value >> 8)
//   inversion — 0 becomes full brightness and full becomes dark (a sink-wired zone)
//   halving   — a stand-in for a perceptual curve: duty is half of the default
//
// Hardware: STM32 alone. Three timer-capable pins (PB0 TIM3_CH3, PB1 TIM3_CH4, PB5 TIM3_CH2),
// nothing wired — every assertion reads the test seam, not a pin level.

#include <Arduino.h>
#include <STM32Board.h>
#include <Outputs/Dimmer/Dimmer.h>

static constexpr uint16_t CTRL_ID = 0x1234;

static uint16_t identity(uint16_t v) { return v; }
static uint16_t invert(uint16_t v)   { return (uint16_t)(0xFFFF - v); }
static uint16_t halve(uint16_t v)    { return (uint16_t)(v >> 1); }

OpenSkyhawk::Dimmer gIdentity(CTRL_ID, PinRef(PB0), identity);
OpenSkyhawk::Dimmer gInverted(CTRL_ID, PinRef(PB1), invert);
OpenSkyhawk::Dimmer gHalved  (CTRL_ID, PinRef(PB5), halve);

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== Dimmer scale_fn ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    gIdentity.configure();
    gInverted.configure();
    gHalved.configure();
    check("all three enabled              ",
          gIdentity.enabled() && gInverted.enabled() && gHalved.enabled());

    gIdentity.onControlPacket(CTRL_ID, 0x8000);
    gInverted.onControlPacket(CTRL_ID, 0x8000);
    gHalved.onControlPacket(CTRL_ID, 0x8000);
    check("identity: 0x8000 -> duty 128   ", gIdentity.lastDuty() == 128);
    check("invert:   0x8000 -> duty 127   ", gInverted.lastDuty() == 127);   // 0xFFFF-0x8000 = 0x7FFF
    check("halve:    0x8000 -> duty 64    ", gHalved.lastDuty() == 64);

    // The extremes are where an inverted zone differs most from the default map.
    gIdentity.onControlPacket(CTRL_ID, 0x0000);
    gInverted.onControlPacket(CTRL_ID, 0x0000);
    check("identity: 0x0000 -> duty 0     ", gIdentity.lastDuty() == 0);
    check("invert:   0x0000 -> duty 255   ", gInverted.lastDuty() == 255);

    gInverted.onControlPacket(CTRL_ID, 0xFFFF);
    check("invert:   0xFFFF -> duty 0     ", gInverted.lastDuty() == 0);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
