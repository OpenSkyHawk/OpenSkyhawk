// SimGateway - UART loopback test
//
// Verifies the public SimGateway::loop() drain path through the configured
// RP2040 UART pins. Requires a jumper from GP0 (UART0 TX) to GP1 (UART0 RX),
// or equivalent pins if setup() is called with a board-specific override.
//
// Flash:
//   pio run -e test_uart_loopback -t upload
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

static void drainLoopUntil(uint32_t timeoutMs, size_t expectedCaptureCount, uint8_t expectedDispatchCount) {
    const uint32_t start = millis();
    while ((millis() - start) < timeoutMs) {
        SimGateway::loop();
        if ((SimGateway::cdcCaptureCount() >= expectedCaptureCount) &&
            (_sgtest_dispatchCount >= expectedDispatchCount)) {
            break;
        }
        delay(1);
    }
}

static void clearUartRx() {
    while (Serial1.available()) {
        Serial1.read();
    }
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

    // Test 1: ASCII bytes travel over configured UART pins and forward to CDC.
    SimGateway::resetParser();
    SimGateway::resetCdcCapture();
    _sgtest_dispatchCount = 0;
    clearUartRx();

    const uint8_t command[] = "ARM_MASTER 1\n";
    Serial1.write(command, sizeof(command) - 1);
    Serial1.flush();
    drainLoopUntil(100, sizeof(command) - 1, 0);

    bool t1 = (_sgtest_dispatchCount == 0) &&
              captureEquals(command, sizeof(command) - 1);

    Serial.print(F("[T1] UART pins forward ASCII to CDC: "));
    Serial.print(t1 ? F("PASS") : F("FAIL"));
    if (!t1) printCapture();
    Serial.println();
    allPass &= t1;

    // Test 2: valid HID frame over configured UART pins is consumed, not forwarded.
    SimGateway::resetParser();
    SimGateway::resetCdcCapture();
    _sgtest_dispatchCount = 0;
    clearUartRx();

    const uint8_t hidFrame[] = {
        0xAA, 0x55,
        (uint8_t)(CTRL_ROLL & 0xFF), (uint8_t)(CTRL_ROLL >> 8),
        0x00, 0x80
    };
    Serial1.write(hidFrame, sizeof(hidFrame));
    Serial1.flush();
    drainLoopUntil(100, 0, 1);

    bool t2 = (_sgtest_dispatchCount == 1) &&
              captureEquals(nullptr, 0);

    Serial.print(F("[T2] UART pins consume HID frame: "));
    Serial.print(t2 ? F("PASS") : F("FAIL"));
    if (!t2) printCapture();
    Serial.println();
    allPass &= t2;

    // Test 3: bad magic over configured UART pins forwards both bytes.
    SimGateway::resetParser();
    SimGateway::resetCdcCapture();
    _sgtest_dispatchCount = 0;
    clearUartRx();

    const uint8_t badMagic[] = {0xAA, 0x42};
    Serial1.write(badMagic, sizeof(badMagic));
    Serial1.flush();
    drainLoopUntil(100, sizeof(badMagic), 0);

    bool t3 = (_sgtest_dispatchCount == 0) &&
              captureEquals(badMagic, sizeof(badMagic));

    Serial.print(F("[T3] UART pins forward bad magic: "));
    Serial.print(t3 ? F("PASS") : F("FAIL"));
    if (!t3) printCapture();
    Serial.println();
    allPass &= t3;

    Serial.println();
    Serial.println(allPass ? F("=== RESULT: PASS ===") : F("=== RESULT: FAIL ==="));
}

void setup() {
    Serial.begin(115200);
    delay(2000);

    SimGateway::setup(Serial1);

    Serial.print(F("=== uart_loopback test GP"));
    Serial.print(SimGateway::DEFAULT_UART_TX_PIN);
    Serial.print(F(" TX -> GP"));
    Serial.print(SimGateway::DEFAULT_UART_RX_PIN);
    Serial.println(F(" RX ==="));
    Serial.println(F("Jumper required: GP0 to GP1 unless setup() pin args are overridden."));

    runTests();
}

void loop() {}
