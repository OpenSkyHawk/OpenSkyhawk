// SimGateway — parser resync test
//
// Verifies that 0xAA followed by a non-0x55 byte does NOT start a HID frame:
// both bytes are forwarded to USB CDC and the parser returns to IDLE. A valid
// HID frame immediately following the bad sequence is parsed correctly.
//
// Flash:
//   pio run -e test_parser_resync -t upload
// Monitor: open USB CDC (115200) on the Pico.

#include <Arduino.h>
#include <SimGateway.h>
#include <HIDControls.h>

extern uint16_t _sgtest_lastControlId;
extern uint16_t _sgtest_lastValue;
extern uint8_t  _sgtest_dispatchCount;

OpenSkyhawk::HIDAxis rollAxis(CTRL_ROLL, 0);

static void runTests() {
    bool allPass = true;

    // ── Test 1: 0xAA + non-0x55 does not trigger setter ──────────────────────
    SimGateway::resetParser();
    _sgtest_dispatchCount = 0;
    bool fired = false;
    fired |= SimGateway::feedByte(0xAA);
    fired |= SimGateway::feedByte(0x42); // not 0x55 — resync
    bool t1 = (!fired && _sgtest_dispatchCount == 0);
    Serial.print(F("[T1] 0xAA + 0x42 no setter: "));
    Serial.println(t1 ? F("PASS") : F("FAIL"));
    allPass &= t1;

    // ── Test 2: parser returns to IDLE after resync ───────────────────────────
    // The byte after the bad sequence (0x42 above) left state=IDLE.
    // Feed a DCS-BIOS byte — should not fire.
    SimGateway::resetParser();
    SimGateway::feedByte(0xAA);
    SimGateway::feedByte(0x42); // resync → IDLE
    bool t2 = !SimGateway::feedByte(0x10); // 0x10 ≤ 0x7F → DCS-BIOS, no fire
    Serial.print(F("[T2] DCS-BIOS after resync no setter: "));
    Serial.println(t2 ? F("PASS") : F("FAIL"));
    allPass &= t2;

    // ── Test 3: valid frame immediately after bad sequence parses correctly ───
    SimGateway::resetParser();
    _sgtest_dispatchCount = 0;
    _sgtest_lastControlId = 0;
    _sgtest_lastValue     = 0;

    // Bad sequence first
    SimGateway::feedByte(0xAA);
    SimGateway::feedByte(0x99); // resync

    // Immediately follow with a valid frame
    const uint8_t frame[] = {
        0xAA, 0x55,
        (uint8_t)(CTRL_ROLL & 0xFF), (uint8_t)(CTRL_ROLL >> 8),
        0x00, 0x40  // value = 0x4000
    };
    bool anyFired = false;
    for (uint8_t i = 0; i < 6; i++) anyFired |= SimGateway::feedByte(frame[i]);

    bool t3 = anyFired && (_sgtest_dispatchCount == 1) &&
              (_sgtest_lastControlId == CTRL_ROLL) && (_sgtest_lastValue == 0x4000);
    Serial.print(F("[T3] Valid frame after resync: fired="));
    Serial.print(anyFired);
    Serial.print(F(" count="));
    Serial.print(_sgtest_dispatchCount);
    Serial.print(F(" val=0x"));
    Serial.print(_sgtest_lastValue, HEX);
    Serial.print(F(": "));
    Serial.println(t3 ? F("PASS") : F("FAIL"));
    allPass &= t3;

    // ── Test 4: 0xAA at end of one drain, 0x55 at start of next (split frame) ─
    SimGateway::resetParser();
    _sgtest_dispatchCount = 0;
    SimGateway::feedByte(0xAA); // GOT_AA state, loop() ends here in production
    // Next "loop()" iteration continues from GOT_AA
    const uint8_t rest[] = {
        0x55,
        (uint8_t)(CTRL_ROLL & 0xFF), (uint8_t)(CTRL_ROLL >> 8),
        0xFF, 0x7F  // value = 0x7FFF
    };
    anyFired = false;
    for (uint8_t i = 0; i < 5; i++) anyFired |= SimGateway::feedByte(rest[i]);
    bool t4 = anyFired && (_sgtest_dispatchCount == 1) && (_sgtest_lastValue == 0x7FFF);
    Serial.print(F("[T4] Split frame across drain boundary: "));
    Serial.println(t4 ? F("PASS") : F("FAIL"));
    allPass &= t4;

    Serial.println();
    Serial.println(allPass ? F("=== RESULT: PASS ===") : F("=== RESULT: FAIL ==="));
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println(F("=== parser_resync test ==="));
    runTests();
}

void loop() {}
