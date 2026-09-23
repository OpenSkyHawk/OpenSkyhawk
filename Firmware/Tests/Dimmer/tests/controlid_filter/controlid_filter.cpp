// Dimmer — controlId filter test
//
// Verifies:
//   A packet for another controlId leaves the duty untouched.
//   The matching controlId still applies after non-matching ones went past.
//
// Hardware: STM32 alone. PB0 (TIM3_CH3), nothing wired.

#include <Arduino.h>
#include <STM32Board.h>
#include <Outputs/Dimmer/Dimmer.h>

static constexpr uint8_t  PIN_PWM  = PB0;
static constexpr uint16_t CTRL_ID  = 0x1234;
static constexpr uint16_t OTHER_ID = 0xBEEF;

OpenSkyhawk::Dimmer gDim(CTRL_ID, PinRef(PIN_PWM));

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== Dimmer controlid_filter ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    gDim.configure();
    check("initial: duty 0, no write        ", gDim.lastDuty() == 0 && gDim.writeCount() == 0);

    gDim.onControlPacket(OTHER_ID, 0xFFFF);
    check("other id (full): duty still 0    ", gDim.lastDuty() == 0);
    check("other id (full): no write        ", gDim.writeCount() == 0);

    gDim.onControlPacket(OTHER_ID, 0x4000);
    check("other id (quarter): still no write", gDim.writeCount() == 0);

    gDim.onControlPacket(CTRL_ID, 0xFFFF);
    check("matching id: duty 255            ", gDim.lastDuty() == 255);
    check("matching id: one write           ", gDim.writeCount() == 1);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
