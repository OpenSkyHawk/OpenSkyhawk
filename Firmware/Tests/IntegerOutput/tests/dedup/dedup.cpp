// IntegerOutput — dedup test
//
// The base only calls apply() when the decoded value changed, so a callback that drives a slow
// display is not re-run on every CTRL_BCAST. DCS-BIOS re-broadcasts full state after SYNC_REQ,
// which is exactly the repeat this suppresses.
//
// Hardware: STM32 alone — logic only, no pins, no CAN.

#include <Arduino.h>
#include <STM32Board.h>
#include <Outputs/IntegerOutput/IntegerOutput.h>

static constexpr uint16_t CTRL_ID = 0x1234;

static uint16_t gCalls = 0, gLast = 0;
static void onValue(uint16_t v) { gCalls++; gLast = v; }

OpenSkyhawk::IntegerOutput gOut(CTRL_ID, onValue);

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== IntegerOutput dedup ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    gOut.onControlPacket(CTRL_ID, 0x1000);
    gOut.onControlPacket(CTRL_ID, 0x1000);
    gOut.onControlPacket(CTRL_ID, 0x1000);
    check("same value x3: called once     ", gCalls == 1);
    check("same value x3: value 0x1000    ", gLast == 0x1000);

    gOut.onControlPacket(CTRL_ID, 0x1001);
    check("changed value: called          ", gCalls == 2);
    check("changed value: 0x1001          ", gLast == 0x1001);

    // Back to the previous value — a change, so it fires again.
    gOut.onControlPacket(CTRL_ID, 0x1000);
    check("back to 0x1000: called         ", gCalls == 3);
    check("back to 0x1000: value 0x1000   ", gLast == 0x1000);

    // And the repeat of that one is suppressed again — no stuck state.
    gOut.onControlPacket(CTRL_ID, 0x1000);
    check("repeat again: suppressed       ", gCalls == 3);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
