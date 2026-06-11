// SimGateway — HID frame parser integration test
//
// Verifies that a valid 6-byte HID frame (0xAA 0x55 + controlId LE + value LE) is
// parsed and dispatched, DCS-BIOS bytes (≤ 0x7F) pass through without triggering
// a setter, and Joystick.Send() is called exactly once after a complete frame.
//
// Uses SIMGATEWAY_TEST build flag — feedByte() and resetParser() are available.
// The _sgtest_* globals capture dispatch details from inside SimGateway.cpp.
//
// Flash:
//   pio run -e test_hid_parser -t upload
// Monitor: open USB CDC (115200) on the Pico.

#include <Arduino.h>
#include <SimGateway.h>
#include <HIDControls.h>

// Capture globals defined in SimGateway.cpp under SIMGATEWAY_TEST
extern uint16_t _sgtest_lastControlId;
extern uint16_t _sgtest_lastValue;
extern uint8_t  _sgtest_dispatchCount;

// A registered axis so the parser has something to dispatch to
OpenSkyhawk::HIDAxis rollAxis(CTRL_ROLL, 0);

static void runTests() {
    bool allPass = true;

    // ── Test 1: DCS-BIOS bytes (≤ 0x7F) do not trigger setter ───────────────
    SimGateway::resetParser();
    _sgtest_dispatchCount = 0;
    bool fired = false;
    for (uint8_t b = 0; b <= 0x7F; b++) {
        fired |= SimGateway::feedByte(b);
        if (b == 0x7F) break; // avoid infinite loop
    }
    bool t1 = (!fired && _sgtest_dispatchCount == 0);
    Serial.print(F("[T1] DCS-BIOS bytes forwarded, no setter: "));
    Serial.println(t1 ? F("PASS") : F("FAIL"));
    allPass &= t1;

    // ── Test 2: Valid HID frame dispatched with correct value ─────────────────
    // Frame: 0xAA 0x55 | controlId=CTRL_ROLL (0x0010 LE) | value=0x8000 LE (centre)
    SimGateway::resetParser();
    _sgtest_dispatchCount = 0;
    _sgtest_lastControlId = 0;
    _sgtest_lastValue     = 0;

    const uint8_t frame[] = {
        0xAA, 0x55,
        (uint8_t)(CTRL_ROLL & 0xFF), (uint8_t)(CTRL_ROLL >> 8),
        0x00, 0x80  // value = 0x8000 = 32768
    };
    bool anyFired = false;
    for (uint8_t i = 0; i < 6; i++) anyFired |= SimGateway::feedByte(frame[i]);

    bool t2a = anyFired;
    bool t2b = (_sgtest_dispatchCount == 1);
    bool t2c = (_sgtest_lastControlId == CTRL_ROLL);
    bool t2d = (_sgtest_lastValue == 0x8000);
    bool t2  = t2a && t2b && t2c && t2d;
    Serial.print(F("[T2] Valid frame dispatched, fired="));
    Serial.print(anyFired);
    Serial.print(F(" count="));
    Serial.print(_sgtest_dispatchCount);
    Serial.print(F(" ctrlId=0x"));
    Serial.print(_sgtest_lastControlId, HEX);
    Serial.print(F(" val=0x"));
    Serial.print(_sgtest_lastValue, HEX);
    Serial.print(F(": "));
    Serial.println(t2 ? F("PASS") : F("FAIL"));
    allPass &= t2;

    // ── Test 3: DCS-BIOS bytes after a frame — parser resets to IDLE ─────────
    SimGateway::resetParser();
    _sgtest_dispatchCount = 0;
    for (uint8_t i = 0; i < 6; i++) SimGateway::feedByte(frame[i]); // send frame
    bool t3 = !SimGateway::feedByte(0x42); // DCS-BIOS byte should not fire
    Serial.print(F("[T3] DCS-BIOS byte after frame no setter: "));
    Serial.println(t3 ? F("PASS") : F("FAIL"));
    allPass &= t3;

    Serial.println();
    Serial.println(allPass ? F("=== RESULT: PASS ===") : F("=== RESULT: FAIL ==="));
}

void setup() {
    Serial.begin(115200);
    delay(2000); // wait for CDC to connect
    Serial.println(F("=== hid_parser test ==="));
    runTests();
}

void loop() {}
