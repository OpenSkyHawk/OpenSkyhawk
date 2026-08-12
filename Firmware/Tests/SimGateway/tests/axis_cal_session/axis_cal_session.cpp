// SimGateway - calibration session and command handling
//
// Drives the protocol the way the client will: build a request, feed it through the inbound
// parser, and assert the reply byte-for-byte off the CDC capture.
//
// Flash is stubbed in SIMGATEWAY_TEST builds, so COMMIT and RESET exercise everything except
// the sector write itself. Persistence across a power cycle is axis_cal_persist's job.
//
// Covers:
//   - HELLO / GET_CAL answered OUTSIDE a session — what lets the client show badges with no dialog
//   - COMMIT / RESET / STREAM_SELECT outside a session are NACKed NO_SESSION, not relayed
//   - SEQ echoed on every reply
//   - SESSION_OPEN / SESSION_ACK, STREAM_SELECT, SESSION_CLOSE
//   - CAL_DATA present and calibrated masks, before and after a commit
//   - COMMIT validation: index, deadzone, ordering — each with its own NACK reason and axis
//   - COMMIT is all-or-nothing across records
//   - RESET clears an axis and needs no following COMMIT
//
// Flash:
//   pio run -e test_axis_cal_session -t upload
// Monitor: open USB CDC (115200) on the Pico.

#include <Arduino.h>
#include <SimGateway.h>

using OpenSkyhawk::CAL_MAX_FRAME;
namespace OS = OpenSkyhawk;

// Two axes declared so presentMask has something to report.
OS::HIDAxis axRoll (CTRL_ROLL,  0);
OS::HIDAxis axPitch(CTRL_PITCH, 1);

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
        Serial.print(F(": FAIL (got 0x")); Serial.print(got, HEX);
        Serial.print(F(", want 0x"));      Serial.print(want, HEX); Serial.println(F(")"));
    }
    g_allPass &= ok;
}

// Send one request and leave its reply in the CDC capture.
static void req(uint8_t type, uint8_t seq, const uint8_t* pay, uint16_t len) {
    uint8_t f[CAL_MAX_FRAME];
    const uint16_t n = OS::calBuildFrame(f, sizeof(f), type, seq, pay, len);
    SimGateway::resetCdcCapture();
    SimGateway::resetUartCapture();
    for (uint16_t i = 0; i < n; ++i) SimGateway::feedCdcByte(f[i]);
}

static uint8_t rxType()          { return SimGateway::cdcCaptureByte(4); }
static uint8_t rxSeq()           { return SimGateway::cdcCaptureByte(5); }
static uint8_t rxPay(uint8_t i)  { return SimGateway::cdcCaptureByte(8 + i); }
static uint16_t rxPay16(uint8_t i) {
    return (uint16_t)(rxPay(i) | ((uint16_t)rxPay(i + 1) << 8));
}

// Build one COMMIT record into buf at offset o.
static void putRec(uint8_t* buf, uint16_t o, uint8_t idx,
                   uint16_t mn, uint16_t ctr, uint16_t mx, uint16_t dz) {
    buf[o] = idx;
    buf[o+1] = (uint8_t)mn;  buf[o+2] = (uint8_t)(mn >> 8);
    buf[o+3] = (uint8_t)ctr; buf[o+4] = (uint8_t)(ctr >> 8);
    buf[o+5] = (uint8_t)mx;  buf[o+6] = (uint8_t)(mx >> 8);
    buf[o+7] = (uint8_t)dz;  buf[o+8] = (uint8_t)(dz >> 8);
}

static void openSession(uint8_t axis) {
    const uint8_t p[1] = { axis };
    req(OS::CAL_T_SESSION_OPEN, 0x01, p, 1);
}

// ── Outside a session ─────────────────────────────────────────────────────────
static void testOutsideSession() {
    SimGateway::calResetForTest();

    req(OS::CAL_T_HELLO, 0x21, nullptr, 0);
    checkEq(F("[A1] HELLO answered with HELLO_ACK"), rxType(), OS::CAL_T_HELLO_ACK);
    checkEq(F("[A2] SEQ echoed"),                    rxSeq(),  0x21);
    checkEq(F("[A3] proto version"),                 rxPay(0), OS::CAL_PROTO_VERSION);
    checkEq(F("[A4] blob version"),                  rxPay(1), OS::CAL_VERSION);
    checkEq(F("[A5] axis slots"),                    rxPay(2), OS::AXIS_CAL_SLOTS);

    req(OS::CAL_T_GET_CAL, 0x22, nullptr, 0);
    checkEq(F("[A6] GET_CAL answered outside a session"), rxType(), OS::CAL_T_CAL_DATA);
    checkEq(F("[A7] CAL_DATA is 82 bytes payload"),
            SimGateway::cdcCaptureCount(), 82 + OS::CAL_ENVELOPE_BYTES);
    checkEq(F("[A8] presentMask = axes 0 and 1"), rxPay(0), 0x03);
    checkEq(F("[A9] nothing calibrated yet"),     rxPay(1), 0x00);
    checkEq(F("[A10] axis 0 controlId is CTRL_ROLL"),  rxPay16(2),  CTRL_ROLL);
    checkEq(F("[A11] axis 1 controlId is CTRL_PITCH"), rxPay16(12), CTRL_PITCH);

    // Session-required commands are consumed and refused, never relayed onward.
    uint8_t c[10]; c[0] = 1; putRec(c, 1, 0, 100, 200, 300, 0);
    req(OS::CAL_T_COMMIT, 0x23, c, sizeof(c));
    checkEq(F("[A12] COMMIT outside a session is NACKed"), rxType(), OS::CAL_T_NACK);
    checkEq(F("[A13] reason NO_SESSION"),  rxPay(1), OS::CAL_NACK_NO_SESSION);
    checkEq(F("[A14] NACK names the type"), rxPay(0), OS::CAL_T_COMMIT);
    checkEq(F("[A15] and it was not relayed to PanelBridge"),
            SimGateway::uartCaptureCount(), 0);
}

// ── Session lifecycle ─────────────────────────────────────────────────────────
static void testLifecycle() {
    SimGateway::calResetForTest();
    check(F("[B1] no session initially"), !SimGateway::calSessionOpen());

    openSession(0);
    checkEq(F("[B2] SESSION_OPEN answered with SESSION_ACK"), rxType(), OS::CAL_T_SESSION_ACK);
    check  (F("[B3] session now open"), SimGateway::calSessionOpen());
    checkEq(F("[B4] timeout advertised as 30000 ms"),
            (uint32_t)rxPay16(0) | ((uint32_t)rxPay16(2) << 16), 30000u);
    checkEq(F("[B5] selected axis echoed"), rxPay(4), 0);
    checkEq(F("[B6] stream axis stored"),   SimGateway::calStreamAxis(), 0);

    const uint8_t sel[1] = { 1 };
    req(OS::CAL_T_STREAM_SELECT, 0x31, sel, 1);
    checkEq(F("[B7] STREAM_SELECT ACKed"),       rxType(), OS::CAL_T_ACK);
    checkEq(F("[B8] ACK names the type"),        rxPay(0), OS::CAL_T_STREAM_SELECT);
    checkEq(F("[B9] stream axis switched"),      SimGateway::calStreamAxis(), 1);

    const uint8_t bad[1] = { 9 };
    req(OS::CAL_T_STREAM_SELECT, 0x32, bad, 1);
    checkEq(F("[B10] out-of-range axis NACKed"), rxType(), OS::CAL_T_NACK);
    checkEq(F("[B11] reason BAD_INDEX"),         rxPay(1), OS::CAL_NACK_BAD_INDEX);
    checkEq(F("[B12] detail names the axis"),    rxPay(2), 9);

    req(OS::CAL_T_KEEPALIVE, 0x33, nullptr, 0);
    checkEq(F("[B13] KEEPALIVE ACKed"), rxType(), OS::CAL_T_ACK);
    check  (F("[B14] session still open"), SimGateway::calSessionOpen());

    req(OS::CAL_T_SESSION_CLOSE, 0x34, nullptr, 0);
    checkEq(F("[B15] SESSION_CLOSE ACKed"), rxType(), OS::CAL_T_ACK);
    check  (F("[B16] session closed"), !SimGateway::calSessionOpen());
    checkEq(F("[B17] stream selection cleared"),
            SimGateway::calStreamAxis(), OS::CAL_AXIS_NONE);
}

// ── COMMIT validation ─────────────────────────────────────────────────────────
static void testCommitValidation() {
    SimGateway::calResetForTest();
    openSession(0);

    uint8_t c[19];

    // Bad axis index.
    c[0] = 1; putRec(c, 1, 8, 100, 200, 300, 0);
    req(OS::CAL_T_COMMIT, 0x41, c, 10);
    checkEq(F("[C1] index 8 NACKed BAD_INDEX"), rxPay(1), OS::CAL_NACK_BAD_INDEX);
    checkEq(F("[C2] detail is the index"),      rxPay(2), 8);

    // Non-zero deadzone: reserved in this protocol version.
    c[0] = 1; putRec(c, 1, 0, 100, 200, 300, 5);
    req(OS::CAL_T_COMMIT, 0x42, c, 10);
    checkEq(F("[C3] deadzone 5 NACKed BAD_DEADZONE"), rxPay(1), OS::CAL_NACK_BAD_DEADZONE);

    // Endpoints out of order.
    c[0] = 1; putRec(c, 1, 0, 300, 200, 100, 0);
    req(OS::CAL_T_COMMIT, 0x43, c, 10);
    checkEq(F("[C4] inverted endpoints NACKed BAD_ORDER"), rxPay(1), OS::CAL_NACK_BAD_ORDER);
    checkEq(F("[C5] detail names the axis"),               rxPay(2), 0);

    // All-or-nothing: a good record followed by a bad one applies neither.
    c[0] = 2;
    putRec(c, 1,  0, 13443, 34728, 50704, 0);   // valid
    putRec(c, 10, 1,   500,   400,   300, 0);   // inverted
    req(OS::CAL_T_COMMIT, 0x44, c, 19);
    checkEq(F("[C6] mixed batch NACKed"), rxType(), OS::CAL_T_NACK);
    checkEq(F("[C7] blamed axis is the bad one"), rxPay(2), 1);

    req(OS::CAL_T_GET_CAL, 0x45, nullptr, 0);
    checkEq(F("[C8] neither axis was applied"), rxPay(1), 0x00);
}

// ── COMMIT and RESET happy paths ──────────────────────────────────────────────
static void testCommitAndReset() {
    SimGateway::calResetForTest();
    openSession(0);

    uint8_t c[19];
    c[0] = 2;
    putRec(c, 1,  0, 13443, 34728, 50704, 0);
    putRec(c, 10, 1, 13058, 33112, 53741, 0);
    req(OS::CAL_T_COMMIT, 0x51, c, 19);
    checkEq(F("[D1] valid COMMIT ACKed"),  rxType(), OS::CAL_T_ACK);
    checkEq(F("[D2] ACK names COMMIT"),    rxPay(0), OS::CAL_T_COMMIT);
    checkEq(F("[D3] SEQ echoed"),          rxSeq(),  0x51);

    req(OS::CAL_T_GET_CAL, 0x52, nullptr, 0);
    checkEq(F("[D4] both axes now calibrated"), rxPay(1), 0x03);
    checkEq(F("[D5] axis 0 min stored"),    rxPay16(4),  13443);
    checkEq(F("[D6] axis 0 centre stored"), rxPay16(6),  34728);
    checkEq(F("[D7] axis 0 max stored"),    rxPay16(8),  50704);

    // The live transform must reflect the commit — centre lands dead centre.
    checkEq(F("[D8] committed calibration is applied"),
            axisCalApply(SimGateway::calibration().axes[0], 34728), 32768);

    // RESET deletes and persists in one message; no following COMMIT.
    const uint8_t r0[1] = { 0 };
    req(OS::CAL_T_RESET, 0x53, r0, 1);
    checkEq(F("[D9] RESET ACKed"), rxType(), OS::CAL_T_ACK);
    req(OS::CAL_T_GET_CAL, 0x54, nullptr, 0);
    checkEq(F("[D10] axis 0 uncalibrated, axis 1 untouched"), rxPay(1), 0x02);

    const uint8_t rAll[1] = { OS::CAL_AXIS_NONE };
    req(OS::CAL_T_RESET, 0x55, rAll, 1);
    req(OS::CAL_T_GET_CAL, 0x56, nullptr, 0);
    checkEq(F("[D11] RESET all clears every axis"), rxPay(1), 0x00);
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println(F("=== SimGateway calibration session test ==="));
    testOutsideSession();
    testLifecycle();
    testCommitValidation();
    testCommitAndReset();
    Serial.println(g_allPass ? F("=== ALL PASS ===") : F("=== FAILURES PRESENT ==="));
}

void loop() {}
