// IntegerOutput — callback test
//
// Verifies:
//   A matching packet invokes the callback once, with the decoded value.
//   A second, different value invokes it again.
//   A nullptr callback is harmless (a sketch may stub one out).
//
// Hardware: STM32 alone — logic only, no pins, no CAN.

#include <Arduino.h>
#include <STM32Board.h>
#include <Outputs/IntegerOutput/IntegerOutput.h>

static constexpr uint16_t CTRL_ID = 0x1234;

static uint16_t gCalls = 0;
static uint16_t gLast  = 0;
static void onValue(uint16_t v) { gCalls++; gLast = v; }

OpenSkyhawk::IntegerOutput gOut(CTRL_ID, onValue);
OpenSkyhawk::IntegerOutput gNullOut(CTRL_ID, nullptr);

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== IntegerOutput callback ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    check("no packet yet: not called      ", gCalls == 0);

    gOut.onControlPacket(CTRL_ID, 0xBEEF);
    check("matching packet: called once   ", gCalls == 1);
    check("matching packet: value passed  ", gLast == 0xBEEF);

    gOut.onControlPacket(CTRL_ID, 0x0001);
    check("new value: called again        ", gCalls == 2);
    check("new value: value passed        ", gLast == 0x0001);

    // Zero is a value like any other, not "no value".
    gOut.onControlPacket(CTRL_ID, 0x0000);
    check("zero value: called             ", gCalls == 3 && gLast == 0x0000);

    // A nullptr callback must not fault.
    gNullOut.onControlPacket(CTRL_ID, 0xFFFF);
    check("nullptr callback: no crash     ", true);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
