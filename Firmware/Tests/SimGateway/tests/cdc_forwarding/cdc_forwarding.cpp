// SimGateway - CDC forwarding test
//
// Verifies the parser forwarding contract:
//   - ordinary PanelBridge -> SimGateway UART bytes are forwarded to USB CDC in order
//   - valid HID frames are consumed and do not forward their bytes to USB CDC
//   - 0xAA followed by a non-0x55 byte forwards both bytes and resumes scanning
//   - ordinary bytes after a valid HID frame still forward
//
// Flash:
//   pio run -e test_cdc_forwarding -t upload
// Monitor: open USB CDC (115200) on the Pico.

#include <Arduino.h>
#include <SimGateway.h>
#include <HIDControls.h>

extern uint8_t _sgtest_dispatchCount;

OpenSkyhawk::HIDAxis rollAxis(CTRL_ROLL, 0);

static bool captureEquals(const uint8_t* expected, size_t length) {
    if (SimGateway::cdcCaptureOverflow()) return false;
    if (SimGateway::cdcCaptureCount() != length) return false;

    for (size_t i = 0; i < length; i++) {
        if (SimGateway::cdcCaptureByte(i) != expected[i]) return false;
    }
    return true;
}

static bool feedBytes(const uint8_t* bytes, size_t length) {
    bool fired = false;
    for (size_t i = 0; i < length; i++) {
        fired |= SimGateway::feedByte(bytes[i]);
    }
    return fired;
}

static void printCapture() {
    Serial.print(F(" captured=["));
    for (size_t i = 0; i < SimGateway::cdcCaptureCount(); i++) {
        if (i) Serial.print(F(" "));
        Serial.print(F("0x"));
        if (SimGateway::cdcCaptureByte(i) < 0x10) Serial.print(F("0"));
        Serial.print(SimGateway::cdcCaptureByte(i), HEX);
    }
    Serial.print(F("]"));
    if (SimGateway::cdcCaptureOverflow()) Serial.print(F(" overflow"));
}

static void runTests() {
    bool allPass = true;

    // Test 1: printable DCS-BIOS command bytes forward in order.
    SimGateway::resetParser();
    SimGateway::resetCdcCapture();
    _sgtest_dispatchCount = 0;

    const uint8_t command[] = "ARM_MASTER 1\n";
    bool fired = feedBytes(command, sizeof(command) - 1);
    bool t1 = !fired && (_sgtest_dispatchCount == 0) &&
              captureEquals(command, sizeof(command) - 1);

    Serial.print(F("[T1] ASCII UART bytes forwarded to CDC: "));
    Serial.print(t1 ? F("PASS") : F("FAIL"));
    if (!t1) printCapture();
    Serial.println();
    allPass &= t1;

    // Test 2: a valid HID frame is consumed, not forwarded to CDC.
    SimGateway::resetParser();
    SimGateway::resetCdcCapture();
    _sgtest_dispatchCount = 0;

    const uint8_t hidFrame[] = {
        0xAA, 0x55,
        (uint8_t)(CTRL_ROLL & 0xFF), (uint8_t)(CTRL_ROLL >> 8),
        0x00, 0x80
    };
    fired = feedBytes(hidFrame, sizeof(hidFrame));
    bool t2 = fired && (_sgtest_dispatchCount == 1) &&
              captureEquals(nullptr, 0);

    Serial.print(F("[T2] Valid HID frame not forwarded to CDC: "));
    Serial.print(t2 ? F("PASS") : F("FAIL"));
    if (!t2) printCapture();
    Serial.println();
    allPass &= t2;

    // Test 3: bad magic prefix forwards both bytes and returns to IDLE.
    SimGateway::resetParser();
    SimGateway::resetCdcCapture();
    _sgtest_dispatchCount = 0;

    const uint8_t badMagic[] = {0xAA, 0x42, 0x43};
    const uint8_t badMagicExpected[] = {0xAA, 0x42, 0x43};
    fired = feedBytes(badMagic, sizeof(badMagic));
    bool t3 = !fired && (_sgtest_dispatchCount == 0) &&
              captureEquals(badMagicExpected, sizeof(badMagicExpected));

    Serial.print(F("[T3] Bad magic forwarded and parser resumed: "));
    Serial.print(t3 ? F("PASS") : F("FAIL"));
    if (!t3) printCapture();
    Serial.println();
    allPass &= t3;

    // Test 4: ordinary bytes after a valid HID frame still forward.
    SimGateway::resetParser();
    SimGateway::resetCdcCapture();
    _sgtest_dispatchCount = 0;

    feedBytes(hidFrame, sizeof(hidFrame));
    SimGateway::resetCdcCapture();
    const uint8_t afterFrame[] = {'Z', '\n'};
    fired = feedBytes(afterFrame, sizeof(afterFrame));
    bool t4 = !fired && captureEquals(afterFrame, sizeof(afterFrame));

    Serial.print(F("[T4] ASCII bytes after HID frame forwarded: "));
    Serial.print(t4 ? F("PASS") : F("FAIL"));
    if (!t4) printCapture();
    Serial.println();
    allPass &= t4;

    Serial.println();
    Serial.println(allPass ? F("=== RESULT: PASS ===") : F("=== RESULT: FAIL ==="));
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println(F("=== cdc_forwarding test ==="));
    runTests();
}

void loop() {}
