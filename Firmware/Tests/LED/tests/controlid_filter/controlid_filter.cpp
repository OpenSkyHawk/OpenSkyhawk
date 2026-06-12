// LED — controlId filter test
//
// Verifies:
//   Non-matching controlId leaves pin unchanged.
//   Correct controlId after a non-matching one still triggers a write.
//
// Hardware: STM32. GPIO PB0. No CAN, no MCP23017.

#include <Arduino.h>
#include <LED.h>

static constexpr uint8_t  TEST_PIN = PB0;
static constexpr uint16_t CTRL_ID  = 0x1234;
static constexpr uint16_t OTHER_ID = 0xBEEF;

OpenSkyhawk::LED gLed(CTRL_ID, 0xFFFF, PinRef(TEST_PIN));

void setup() {
    Serial.begin(115200);
    while (!Serial) {}
    Serial.println("=== LED controlid_filter ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        Serial.print(label);
        Serial.println(ok ? ": PASS" : ": FAIL");
    };

    gLed.configure(); // pin starts LOW (off)
    check("Initial: pin LOW", digitalRead(TEST_PIN) == LOW);

    // Non-matching ID — pin must not change
    gLed.onControlPacket(OTHER_ID, 0xFFFF);
    check("Non-matching ID: pin still LOW", digitalRead(TEST_PIN) == LOW);

    // Non-matching ID with zero value — still no effect
    gLed.onControlPacket(OTHER_ID, 0x0000);
    check("Non-matching ID (value=0): pin still LOW", digitalRead(TEST_PIN) == LOW);

    // Correct ID — now the write fires
    gLed.onControlPacket(CTRL_ID, 0xFFFF);
    check("Correct ID: pin HIGH (on)", digitalRead(TEST_PIN) == HIGH);

    // Non-matching ID again — pin must stay HIGH (write is not triggered)
    gLed.onControlPacket(OTHER_ID, 0x0000);
    check("Non-matching after on: pin still HIGH", digitalRead(TEST_PIN) == HIGH);

    // Correct ID off
    gLed.onControlPacket(CTRL_ID, 0x0000);
    check("Correct ID, value=0: pin LOW (off)", digitalRead(TEST_PIN) == LOW);

    Serial.println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
