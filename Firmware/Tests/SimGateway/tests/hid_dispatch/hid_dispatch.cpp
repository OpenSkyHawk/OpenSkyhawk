// SimGateway — HID dispatch routing test
//
// Verifies that:
//   - HIDAxis dispatch fires on controlId match with correct value
//   - HIDButton dispatch fires on controlId match
//   - Non-matching controlId does NOT trigger any setter
//   - feedByte() returns false (no setter fired) when non-matching frame received
//
// Flash:
//   pio run -e test_hid_dispatch -t upload
// Monitor: open USB CDC (115200) on the Pico.

#include <Arduino.h>
#include <SimGateway.h>
#include <HIDControls.h>

extern uint16_t _sgtest_lastControlId;
extern uint16_t _sgtest_lastValue;
extern uint8_t  _sgtest_dispatchCount;

// Register one axis and one button
OpenSkyhawk::HIDAxis  rollAxis(CTRL_ROLL,    0); // axis index 0
OpenSkyhawk::HIDAxis  pitchAxis(CTRL_PITCH,  1); // axis index 1
OpenSkyhawk::HIDButton trigger(CTRL_TRIGGER, 0); // button index 0

static bool feedFrame(uint16_t controlId, uint16_t value) {
    const uint8_t frame[] = {
        0xAA, 0x55,
        (uint8_t)(controlId & 0xFF), (uint8_t)(controlId >> 8),
        (uint8_t)(value     & 0xFF), (uint8_t)(value     >> 8)
    };
    bool fired = false;
    for (uint8_t i = 0; i < 6; i++) fired |= SimGateway::feedByte(frame[i]);
    return fired;
}

static void runTests() {
    bool allPass = true;

    // ── Test 1: axis controlId match dispatches correct value ─────────────────
    SimGateway::resetParser();
    _sgtest_dispatchCount = 0;
    _sgtest_lastControlId = 0;
    _sgtest_lastValue     = 0;

    bool fired = feedFrame(CTRL_ROLL, 0xAAAA);
    bool t1 = fired && (_sgtest_dispatchCount == 1) &&
              (_sgtest_lastControlId == CTRL_ROLL) && (_sgtest_lastValue == 0xAAAA);
    Serial.print(F("[T1] CTRL_ROLL dispatched, val=0x"));
    Serial.print(_sgtest_lastValue, HEX);
    Serial.print(F(": "));
    Serial.println(t1 ? F("PASS") : F("FAIL"));
    allPass &= t1;

    // ── Test 2: second axis dispatches independently ──────────────────────────
    SimGateway::resetParser();
    _sgtest_dispatchCount = 0;
    fired = feedFrame(CTRL_PITCH, 0x1234);
    bool t2 = fired && (_sgtest_lastControlId == CTRL_PITCH) && (_sgtest_lastValue == 0x1234);
    Serial.print(F("[T2] CTRL_PITCH dispatched: "));
    Serial.println(t2 ? F("PASS") : F("FAIL"));
    allPass &= t2;

    // ── Test 3: button controlId match dispatches ─────────────────────────────
    SimGateway::resetParser();
    _sgtest_dispatchCount = 0;
    fired = feedFrame(CTRL_TRIGGER, 1); // pressed
    bool t3 = fired && (_sgtest_lastControlId == CTRL_TRIGGER) && (_sgtest_lastValue == 1);
    Serial.print(F("[T3] CTRL_TRIGGER pressed dispatched: "));
    Serial.println(t3 ? F("PASS") : F("FAIL"));
    allPass &= t3;

    SimGateway::resetParser();
    _sgtest_dispatchCount = 0;
    fired = feedFrame(CTRL_TRIGGER, 0); // released
    bool t3b = fired && (_sgtest_lastValue == 0);
    Serial.print(F("[T3b] CTRL_TRIGGER released dispatched: "));
    Serial.println(t3b ? F("PASS") : F("FAIL"));
    allPass &= t3b;

    // ── Test 4: non-matching controlId does not fire any setter ───────────────
    SimGateway::resetParser();
    _sgtest_dispatchCount = 0;
    fired = feedFrame(0x00FF, 0x1234); // no registered handler for 0x00FF
    bool t4 = (!fired && _sgtest_dispatchCount == 0);
    Serial.print(F("[T4] Unknown controlId no setter: "));
    Serial.println(t4 ? F("PASS") : F("FAIL"));
    allPass &= t4;

    // ── Test 5: multiple frames, only matching one dispatches ─────────────────
    SimGateway::resetParser();
    _sgtest_dispatchCount = 0;
    feedFrame(0x0001, 0xBEEF); // unregistered — no fire
    feedFrame(CTRL_ROLL, 0x5678); // registered — fires
    feedFrame(0x0002, 0xDEAD); // unregistered — no fire
    bool t5 = (_sgtest_dispatchCount == 1) && (_sgtest_lastValue == 0x5678);
    Serial.print(F("[T5] Only matching frame dispatched, count="));
    Serial.print(_sgtest_dispatchCount);
    Serial.print(F(": "));
    Serial.println(t5 ? F("PASS") : F("FAIL"));
    allPass &= t5;

    Serial.println();
    Serial.println(allPass ? F("=== RESULT: PASS ===") : F("=== RESULT: FAIL ==="));
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println(F("=== hid_dispatch test ==="));
    runTests();
}

void loop() {}
