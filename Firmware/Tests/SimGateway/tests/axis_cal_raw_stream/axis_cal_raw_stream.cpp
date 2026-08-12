// SimGateway - raw axis streaming during a calibration session
//
// While a session is open the gateway streams the axis under calibration as RAW frames on
// CDC. Only that axis: the dialog calibrates one at a time, so streaming the other seven
// would be bytes nobody reads.
//
// The assertion that matters most is the last group. The fork sits inside the HID dispatch
// walk, and the contract there is one HID report per drain regardless of how many frames
// arrived. If streaming perturbed `fired` or the dispatch count, the joystick report rate
// would change *only while calibrating* — which is exactly the "changes what DCS sees
// mid-session" failure the whole transport design was chosen to avoid.
//
// Covers:
//   - no RAW without an open session
//   - RAW only for the selected axis; other registered axes and buttons emit nothing
//   - STREAM_SELECT switches the streamed axis; 0xFF silences it
//   - payload carries pre-transform raw and post-calibration cal from one sample
//   - SEQ is a free-running per-session counter, reset on SESSION_OPEN
//   - dispatch behaviour is byte-identical with and without a session
//
// Flash:
//   pio run -e test_axis_cal_raw_stream -t upload
// Monitor: open USB CDC (115200) on the Pico.

#include <Arduino.h>
#include <SimGateway.h>

namespace OS = OpenSkyhawk;
using OS::CAL_MAX_FRAME;

extern uint8_t _sgtest_dispatchCount;   // defined in SimGateway.cpp under SIMGATEWAY_TEST

OS::HIDAxis   axRoll (CTRL_ROLL,    0);
OS::HIDAxis   axPitch(CTRL_PITCH,   1);
OS::HIDButton btn    (CTRL_TRIGGER, 0);

// Measured on the AxisBench thumbstick — used so `cal` differs visibly from `raw`.
static const OS::AxisCal ROLL_CAL = { 13443, 34728, 50704, 0 };

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

// Inbound calibration request (host -> device).
static void req(uint8_t type, uint8_t seq, const uint8_t* pay, uint16_t len) {
    uint8_t f[CAL_MAX_FRAME];
    const uint16_t n = OS::calBuildFrame(f, sizeof(f), type, seq, pay, len);
    for (uint16_t i = 0; i < n; ++i) SimGateway::feedCdcByte(f[i]);
}

static void openSession(uint8_t axis) {
    const uint8_t p[1] = { axis };
    req(OS::CAL_T_SESSION_OPEN, 0x01, p, 1);
}

static void streamSelect(uint8_t axis) {
    const uint8_t p[1] = { axis };
    req(OS::CAL_T_STREAM_SELECT, 0x02, p, 1);
}

// One HID frame arriving from PanelBridge (device <- UART). Returns whether a setter fired.
static bool feedHidFrame(uint16_t controlId, uint16_t value) {
    const uint8_t f[6] = {
        0xAA, 0x55,
        (uint8_t)(controlId & 0xFF), (uint8_t)(controlId >> 8),
        (uint8_t)(value     & 0xFF), (uint8_t)(value     >> 8)
    };
    bool fired = false;
    for (uint8_t i = 0; i < sizeof(f); ++i) fired |= SimGateway::feedByte(f[i]);
    return fired;
}

static uint8_t rxType()         { return SimGateway::cdcCaptureByte(4); }
static uint8_t rxSeq()          { return SimGateway::cdcCaptureByte(5); }
static uint8_t rxPay(uint8_t i) { return SimGateway::cdcCaptureByte(8 + i); }
static uint16_t rxPay16(uint8_t i) {
    return (uint16_t)(rxPay(i) | ((uint16_t)rxPay(i + 1) << 8));
}

// ── No session, no stream ─────────────────────────────────────────────────────
static void testSilentWithoutSession() {
    SimGateway::calResetForTest();
    SimGateway::resetCdcCapture();

    const bool fired = feedHidFrame(CTRL_ROLL, 34728);
    check  (F("[A1] axis still dispatches with no session"), fired);
    checkEq(F("[A2] but emits no RAW"), SimGateway::cdcCaptureCount(), 0);
}

// ── Only the selected axis streams ────────────────────────────────────────────
static void testSelectedAxisOnly() {
    SimGateway::calResetForTest();
    openSession(0);

    SimGateway::resetCdcCapture();
    feedHidFrame(CTRL_ROLL, 34728);
    checkEq(F("[B1] selected axis emits one RAW frame"),
            SimGateway::cdcCaptureCount(), OS::CAL_ENVELOPE_BYTES + 5);
    checkEq(F("[B2] frame type is RAW"), rxType(), OS::CAL_T_RAW);
    checkEq(F("[B3] payload names axis 0"), rxPay(0), 0);

    SimGateway::resetCdcCapture();
    feedHidFrame(CTRL_PITCH, 33112);
    checkEq(F("[B4] unselected axis emits nothing"), SimGateway::cdcCaptureCount(), 0);

    SimGateway::resetCdcCapture();
    feedHidFrame(CTRL_TRIGGER, 1);
    checkEq(F("[B5] button frames emit nothing"), SimGateway::cdcCaptureCount(), 0);

    // Switching follows STREAM_SELECT.
    streamSelect(1);
    SimGateway::resetCdcCapture();
    feedHidFrame(CTRL_PITCH, 33112);
    checkEq(F("[B6] after STREAM_SELECT, axis 1 streams"), rxPay(0), 1);

    SimGateway::resetCdcCapture();
    feedHidFrame(CTRL_ROLL, 34728);
    checkEq(F("[B7] and axis 0 has gone quiet"), SimGateway::cdcCaptureCount(), 0);

    // 0xFF silences the stream without closing the session.
    streamSelect(OS::CAL_AXIS_NONE);
    SimGateway::resetCdcCapture();
    feedHidFrame(CTRL_ROLL,  34728);
    feedHidFrame(CTRL_PITCH, 33112);
    checkEq(F("[B8] 0xFF silences all streaming"), SimGateway::cdcCaptureCount(), 0);
    check  (F("[B9] session is still open"), SimGateway::calSessionOpen());
}

// ── Payload carries both values from one sample ───────────────────────────────
static void testPayloadValues() {
    SimGateway::calResetForTest();

    // Uncalibrated: cal == raw, since the transform is identity.
    openSession(0);
    SimGateway::resetCdcCapture();
    feedHidFrame(CTRL_ROLL, 34728);
    checkEq(F("[C1] uncalibrated raw"), rxPay16(1), 34728);
    checkEq(F("[C2] uncalibrated cal equals raw"), rxPay16(3), 34728);

    // Calibrated: raw is unchanged, cal is the transformed value. Centre must land on 32768,
    // which is what proves the two fields are genuinely different values from one sample.
    OS::CalBlob blob;
    OS::calBlobClear(blob);
    blob.axes[0] = ROLL_CAL;
    SimGateway::calSetForTest(blob);

    SimGateway::resetCdcCapture();
    feedHidFrame(CTRL_ROLL, 34728);
    checkEq(F("[C3] raw is still the pre-transform value"), rxPay16(1), 34728);
    checkEq(F("[C4] cal is the calibrated value"),          rxPay16(3), 32768);

    SimGateway::resetCdcCapture();
    feedHidFrame(CTRL_ROLL, ROLL_CAL.max);
    checkEq(F("[C5] at the rail, raw is the sensor reading"), rxPay16(1), ROLL_CAL.max);
    checkEq(F("[C6] and cal is full scale"),                  rxPay16(3), 65535);
}

// ── SEQ is a free-running per-session counter ─────────────────────────────────
static void testSeqCounter() {
    SimGateway::calResetForTest();
    openSession(0);

    SimGateway::resetCdcCapture();
    feedHidFrame(CTRL_ROLL, 30000);
    const uint8_t first = rxSeq();
    checkEq(F("[D1] first sample of a session is SEQ 0"), first, 0);

    SimGateway::resetCdcCapture();
    feedHidFrame(CTRL_ROLL, 30001);
    checkEq(F("[D2] SEQ increments per sample"), rxSeq(), 1);

    SimGateway::resetCdcCapture();
    feedHidFrame(CTRL_ROLL, 30002);
    checkEq(F("[D3] and again"), rxSeq(), 2);

    // A new session restarts the count, so a client can tell sessions apart.
    req(OS::CAL_T_SESSION_CLOSE, 0x03, nullptr, 0);
    openSession(0);
    SimGateway::resetCdcCapture();
    feedHidFrame(CTRL_ROLL, 30003);
    checkEq(F("[D4] a new session restarts SEQ at 0"), rxSeq(), 0);
}

// ── Streaming must not perturb HID dispatch ───────────────────────────────────
static void testDispatchUnchanged() {
    // Baseline: no session.
    SimGateway::calResetForTest();
    _sgtest_dispatchCount = 0;
    bool firedNoSession = false;
    for (uint8_t i = 0; i < 5; ++i) firedNoSession |= feedHidFrame(CTRL_ROLL, 30000 + i);
    const uint8_t countNoSession = _sgtest_dispatchCount;

    // Same traffic with a session open and axis 0 streaming.
    SimGateway::calResetForTest();
    openSession(0);
    _sgtest_dispatchCount = 0;
    SimGateway::resetCdcCapture();
    bool firedSession = false;
    for (uint8_t i = 0; i < 5; ++i) firedSession |= feedHidFrame(CTRL_ROLL, 30000 + i);
    const uint8_t countSession = _sgtest_dispatchCount;

    check  (F("[E1] fired is identical"), firedNoSession == firedSession);
    checkEq(F("[E2] dispatch count is identical"), countSession, countNoSession);
    checkEq(F("[E3] and streaming did happen"),
            SimGateway::cdcCaptureCount(), 5 * (OS::CAL_ENVELOPE_BYTES + 5));

    // The specific mistake E1/E2 cannot see: the fork sending its own HID reports. Report
    // sends belong to loop(), not to the dispatch walk, so feeding frames directly must
    // produce none at all — with or without a session.
    SimGateway::calResetForTest();
    SimGateway::resetHidSendCount();
    for (uint8_t i = 0; i < 5; ++i) feedHidFrame(CTRL_ROLL, 30000 + i);
    checkEq(F("[E4] no session: dispatch sends no reports"), SimGateway::hidSendCount(), 0);

    openSession(0);
    SimGateway::resetHidSendCount();
    for (uint8_t i = 0; i < 5; ++i) feedHidFrame(CTRL_ROLL, 30000 + i);
    checkEq(F("[E5] streaming sends no reports either"), SimGateway::hidSendCount(), 0);
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println(F("=== SimGateway raw axis streaming test ==="));
    testSilentWithoutSession();
    testSelectedAxisOnly();
    testPayloadValues();
    testSeqCounter();
    testDispatchUnchanged();
    Serial.println(g_allPass ? F("=== ALL PASS ===") : F("=== FAILURES PRESENT ==="));
}

void loop() {}
