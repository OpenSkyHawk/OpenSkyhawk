// LED — dedup test (audit #10)
//
// onControlPacket() skips the pin write when the decoded on/off state is unchanged
// (_lastOn / _hasState). A repeated value writes the pin exactly once; a real change writes again.
// Verified via the LED_TEST writeCount() seam — a *stateless* LED would fail this, whereas the
// existing on_off test (which only sends changing values) would not catch the regression.
//
// Hardware: STM32. GPIO PB0 only. No CAN, no MCP23017.

#include <Arduino.h>
#include <STM32Board.h>
#include <Outputs/LED/LED.h>

static constexpr uint8_t  TEST_PIN = PB0;
static constexpr uint16_t CTRL_ID  = 0x1234;
static constexpr uint16_t MASK     = 0xFFFF;

OpenSkyhawk::LED gLed(CTRL_ID, MASK, PinRef(TEST_PIN));

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== LED dedup ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    gLed.configure();
    check("after configure: 0 packet-writes", gLed.writeCount() == 0);

    gLed.onControlPacket(CTRL_ID, 0x0001);             // first ON → write
    check("first ON: 1 write", gLed.writeCount() == 1 && digitalRead(TEST_PIN) == HIGH);

    gLed.onControlPacket(CTRL_ID, 0x0001);             // same → deduped
    gLed.onControlPacket(CTRL_ID, 0x0001);             // same again
    check("repeated ON: still 1 write (deduped)", gLed.writeCount() == 1);

    gLed.onControlPacket(CTRL_ID, 0x0000);             // change → write
    check("OFF: 2 writes", gLed.writeCount() == 2 && digitalRead(TEST_PIN) == LOW);

    gLed.onControlPacket(CTRL_ID, 0x0000);             // same → deduped
    check("repeated OFF: still 2 writes (deduped)", gLed.writeCount() == 2);

    gLed.onControlPacket(0x9999, 0xFFFF);              // controlId mismatch → ignored
    check("non-matching controlId: no write", gLed.writeCount() == 2);

    gLed.onControlPacket(CTRL_ID, 0x0001);             // back ON → write (3)
    gLed.onControlPacket(CTRL_ID, 0x00FF);             // different value, still ON (mask 0xFFFF) → deduped
    check("different nonzero value, same ON state: deduped", gLed.writeCount() == 3);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
