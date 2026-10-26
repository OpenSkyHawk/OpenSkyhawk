// SimGateway - non-blocking host-bound relay (#355)
//
// The UART→host relay never blocks: a line is written whole when the CDC FIFO has room for
// all of it, otherwise dropped and counted. The FIFO's free space is injected with
// cdcSetRoomForTest(), so the drop path is reachable without a host that stops reading.
//
// Covers:
//   - nothing is written until the line's '\n'
//   - a line that exactly fits is written; one byte short and it is dropped whole, not truncated
//   - a dropped line leaves no residue: the next line forwards untouched
//   - non-ASCII (control bytes, '\r', >0x7E) makes the line corrupt — dropped, counted;
//     before a line starts it is dropped on its own (#367)
//   - a line longer than RELAY_LINE_MAX is dropped as overlong; RELAY_LINE_MAX itself fits
//   - a HID frame inside a line is consumed and the line still assembles around it
//   - a calibration frame emitted mid-line goes out whole, ahead of the line — never inside it
//   - a calibration frame with no room is dropped whole and counted
//
// Flash:
//   pio run -e test_cdc_relay -t upload
// Monitor: open USB CDC (115200) on the Pico.

#include <Arduino.h>
#include <SimGateway.h>
#include <HIDControls.h>

namespace OS = OpenSkyhawk;

extern uint8_t _sgtest_dispatchCount;

OS::HIDAxis rollAxis(CTRL_ROLL, 0);

static bool g_allPass = true;

static void check(const __FlashStringHelper* label, bool cond) {
    Serial.print(label);
    Serial.println(cond ? F(": PASS") : F(": FAIL"));
    g_allPass &= cond;
}

static void checkEq(const __FlashStringHelper* label, uint32_t got, uint32_t want) {
    const bool ok = (got == want);
    Serial.print(label);
    if (ok) Serial.println(F(": PASS"));
    else {
        Serial.print(F(": FAIL (got ")); Serial.print(got);
        Serial.print(F(", want "));      Serial.print(want); Serial.println(F(")"));
    }
    g_allPass &= ok;
}

static void fresh() {
    SimGateway::resetParser();
    SimGateway::resetCdcCapture();
    SimGateway::diagResetForTest();
    SimGateway::cdcSetRoomForTest(-1);
    SimGateway::calResetForTest();
    _sgtest_dispatchCount = 0;
}

static void feed(const uint8_t* b, size_t n) {
    for (size_t i = 0; i < n; ++i) SimGateway::feedByte(b[i]);
}
static void feedStr(const char* s) { feed(reinterpret_cast<const uint8_t*>(s), strlen(s)); }

static bool captureIs(const char* s) {
    const size_t n = strlen(s);
    if (SimGateway::cdcCaptureOverflow() || SimGateway::cdcCaptureCount() != n) return false;
    for (size_t i = 0; i < n; ++i) {
        if (SimGateway::cdcCaptureByte(i) != (uint8_t)s[i]) return false;
    }
    return true;
}

static void hidRoll(uint16_t v) {
    const uint8_t f[] = { 0xAA, 0x55, (uint8_t)(CTRL_ROLL & 0xFF), (uint8_t)(CTRL_ROLL >> 8),
                          (uint8_t)(v & 0xFF), (uint8_t)(v >> 8) };
    feed(f, sizeof(f));
}

static void calReq(uint8_t type, uint8_t seq, const uint8_t* pay, uint16_t len) {
    uint8_t f[OS::CAL_MAX_FRAME];
    const uint16_t n = OS::calBuildFrame(f, sizeof(f), type, seq, pay, len);
    for (uint16_t i = 0; i < n; ++i) SimGateway::feedCdcByte(f[i]);
}

// ── Whole line or nothing ─────────────────────────────────────────────────────
static void testWholeLine() {
    fresh();
    feedStr("ARM_MASTER 1");
    checkEq(F("[A1] nothing written before the newline"), SimGateway::cdcCaptureCount(), 0);
    feedStr("\n");
    check(F("[A2] whole line written on the newline"), captureIs("ARM_MASTER 1\n"));

    fresh();
    SimGateway::cdcSetRoomForTest(13);   // "ARM_MASTER 1\n" is exactly 13
    feedStr("ARM_MASTER 1\n");
    check(F("[A3] line that exactly fits is written"), captureIs("ARM_MASTER 1\n"));

    fresh();
    SimGateway::cdcSetRoomForTest(12);
    feedStr("ARM_MASTER 1\n");
    checkEq(F("[A4] one byte short: nothing written (no truncation)"), SimGateway::cdcCaptureCount(), 0);
    checkEq(F("[A5] and it is counted as dropped"), SimGateway::diagnostics().linesDropped, 1);

    SimGateway::cdcSetRoomForTest(-1);
    feedStr("GEAR 0\n");
    check(F("[A6] next line forwards untouched after a drop"), captureIs("GEAR 0\n"));
    checkEq(F("[A7] drop counter unchanged by a good line"), SimGateway::diagnostics().linesDropped, 1);
}

// ── Corrupt and overlong ──────────────────────────────────────────────────────
static void testCorruptAndOverlong() {
    fresh();
    const uint8_t ctl[] = { 'A', 0x01, 'B', '\n' };
    feed(ctl, sizeof(ctl));
    checkEq(F("[B1] control byte: line not relayed"), SimGateway::cdcCaptureCount(), 0);
    checkEq(F("[B2] counted as corrupt"), SimGateway::diagnostics().linesCorrupt, 1);

    feedStr("A\r\n");
    checkEq(F("[B3] CR is not part of the contract: corrupt"), SimGateway::diagnostics().linesCorrupt, 2);

    const uint8_t hi[] = { 'A', 0x80, '\n' };
    feed(hi, sizeof(hi));
    checkEq(F("[B4] byte > 0x7E: corrupt"), SimGateway::diagnostics().linesCorrupt, 3);
    checkEq(F("[B5] none of them reached the host"), SimGateway::cdcCaptureCount(), 0);

    // Noise before a line starts — PanelBridge's UART coming up after a reset (#367) — is dropped
    // on its own; the line after it is relayed whole.
    fresh();
    const uint8_t lead[] = { 0xF8, 0x00, '_', 'X', ' ', '1', '\n' };
    feed(lead, sizeof(lead));
    check(F("[B11] leading noise dropped, the line relayed"), captureIs("_X 1\n"));
    checkEq(F("[B12] and not counted as corrupt"), SimGateway::diagnostics().linesCorrupt, 0);

    fresh();
    for (uint16_t i = 0; i < SimGateway::RELAY_LINE_MAX + 5; ++i) SimGateway::feedByte('X');
    feedStr("\n");
    checkEq(F("[B6] overlong line not relayed"), SimGateway::cdcCaptureCount(), 0);
    checkEq(F("[B7] counted as overlong"), SimGateway::diagnostics().linesOverlong, 1);
    feedStr("OK\n");
    check(F("[B8] next line forwards after an overlong one"), captureIs("OK\n"));

    fresh();
    for (uint16_t i = 0; i < SimGateway::RELAY_LINE_MAX - 1; ++i) SimGateway::feedByte('Y');
    feedStr("\n");
    checkEq(F("[B9] RELAY_LINE_MAX bytes including newline still fits"),
            SimGateway::cdcCaptureCount(), SimGateway::RELAY_LINE_MAX);
    checkEq(F("[B10] and is not counted overlong"), SimGateway::diagnostics().linesOverlong, 0);
}

// ── Interleaving ──────────────────────────────────────────────────────────────
static void testInterleaving() {
    fresh();
    feedStr("AB");
    hidRoll(0x8000);
    feedStr("C\n");
    checkEq(F("[C1] HID frame inside a line still dispatches"), _sgtest_dispatchCount, 1);
    check(F("[C2] and the line assembles around it"), captureIs("ABC\n"));

    // A RAW frame is emitted while the line is half-assembled. Before #355 its bytes would
    // have gone out between "AB" and "C\n"; now the line is buffered, so the frame goes first.
    fresh();
    const uint8_t open[1] = { 0 };   // stream axis 0
    calReq(OS::CAL_T_SESSION_OPEN, 0x01, open, 1);
    SimGateway::resetCdcCapture();
    feedStr("AB");
    hidRoll(0x1234);
    feedStr("C\n");
    const size_t raw = OS::CAL_ENVELOPE_BYTES + 5;
    checkEq(F("[C3] RAW frame + line, nothing else"), SimGateway::cdcCaptureCount(), raw + 4);
    checkEq(F("[C4] RAW frame first (magic)"), SimGateway::cdcCaptureByte(0), OS::CAL_FRAME_MAGIC[0]);
    checkEq(F("[C5] RAW type"), SimGateway::cdcCaptureByte(4), OS::CAL_T_RAW);
    check(F("[C6] line intact after the frame"),
          SimGateway::cdcCaptureByte(raw)     == 'A' && SimGateway::cdcCaptureByte(raw + 1) == 'B' &&
          SimGateway::cdcCaptureByte(raw + 2) == 'C' && SimGateway::cdcCaptureByte(raw + 3) == '\n');
}

// ── Calibration frames ────────────────────────────────────────────────────────
static void testFrameDrop() {
    fresh();
    SimGateway::cdcSetRoomForTest(OS::CAL_ENVELOPE_BYTES + 5);   // HELLO_ACK needs 16
    calReq(OS::CAL_T_HELLO, 0x07, nullptr, 0);
    checkEq(F("[D1] HELLO_ACK with no room: nothing written"), SimGateway::cdcCaptureCount(), 0);
    checkEq(F("[D2] counted as a dropped frame"), SimGateway::diagnostics().framesDropped, 1);
    checkEq(F("[D3] not counted as a dropped line"), SimGateway::diagnostics().linesDropped, 0);

    SimGateway::cdcSetRoomForTest(OS::CAL_ENVELOPE_BYTES + 6);
    calReq(OS::CAL_T_HELLO, 0x08, nullptr, 0);
    checkEq(F("[D4] room for exactly the frame: written whole"),
            SimGateway::cdcCaptureCount(), OS::CAL_ENVELOPE_BYTES + 6);
}

static void runTests() {
    testWholeLine();
    testCorruptAndOverlong();
    testInterleaving();
    testFrameDrop();
    SimGateway::cdcSetRoomForTest(-1);

    Serial.println();
    Serial.println(g_allPass ? F("=== RESULT: PASS ===") : F("=== RESULT: FAIL ==="));
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println(F("=== cdc_relay test ==="));
    runTests();
}

void loop() {}
