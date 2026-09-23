// IntegerOutput — controlId filter test
//
// Verifies:
//   A packet for another controlId never reaches the callback.
//   The matching controlId still fires after non-matching ones went past.
//
// Hardware: STM32 alone — logic only, no pins, no CAN.

#include <Arduino.h>
#include <STM32Board.h>
#include <Outputs/IntegerOutput/IntegerOutput.h>

static constexpr uint16_t CTRL_ID  = 0x1234;
static constexpr uint16_t OTHER_ID = 0xBEEF;

static uint16_t gCalls = 0, gLast = 0;
static void onValue(uint16_t v) { gCalls++; gLast = v; }

OpenSkyhawk::IntegerOutput gOut(CTRL_ID, onValue);

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== IntegerOutput controlid_filter ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    gOut.onControlPacket(OTHER_ID, 0xAAAA);
    check("other id: not called           ", gCalls == 0);

    gOut.onControlPacket(OTHER_ID, 0x0000);
    check("other id (zero): not called    ", gCalls == 0);

    gOut.onControlPacket(CTRL_ID, 0xAAAA);
    check("matching id: called            ", gCalls == 1);
    check("matching id: value passed      ", gLast == 0xAAAA);

    // A non-matching packet in between must not disturb the dedup state either.
    gOut.onControlPacket(OTHER_ID, 0x5555);
    gOut.onControlPacket(CTRL_ID, 0xAAAA);
    check("repeat after other id: no call ", gCalls == 1);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
