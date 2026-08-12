// SimGateway - calibration frame parser: accept and reject paths
//
// The calibration parser runs on every inbound byte, always — HELLO and GET_CAL are answered
// outside a session, so there is no point at which it can stop looking. That makes one
// property load-bearing: a candidate is consumed ONLY if it passes magic, LEN-matches-TYPE
// and CRC. Anything failing any of them must reach PanelBridge byte-for-byte, in order.
//
// If that property breaks, the symptom is DCS-BIOS traffic quietly disappearing — which is
// exactly the kind of fault that looks like a hardware problem. So these assertions are
// about what reaches the UART, not about what the parser thinks it did.
//
// This scenario also covers what an earlier design called "entry". There is no separate entry
// magic any more: the 8-byte magic became an ordinary frame, so magic handling is just the
// first four bytes of the parser and is tested here rather than in its own scenario.
//
// Covers:
//   - a valid frame is consumed and contributes nothing to the UART
//   - LEN that does not match its TYPE is rejected before the payload is buffered
//   - a LEN of 65535 on a valid type does not make the parser wait for 65535 bytes
//   - bad CRC is rejected and every consumed byte is handed back in order
//   - partial magic followed by a mismatch flushes, and a mismatching 0xAA restarts a match
//   - an unknown TYPE is rejected at the length gate
//   - plain DCS-BIOS text passes through byte-identical
//
// Flash:
//   pio run -e test_axis_cal_framing -t upload
// Monitor: open USB CDC (115200) on the Pico.

#include <Arduino.h>
#include <SimGateway.h>

using OpenSkyhawk::CAL_ENVELOPE_BYTES;
using OpenSkyhawk::CAL_MAX_FRAME;

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

// Feed a byte sequence through the inbound parser.
static void feed(const uint8_t* b, size_t n) {
    for (size_t i = 0; i < n; ++i) SimGateway::feedCdcByte(b[i]);
}

// True if the UART capture holds exactly these bytes, in this order.
static bool uartIs(const uint8_t* want, size_t n) {
    if (SimGateway::uartCaptureCount() != n) return false;
    for (size_t i = 0; i < n; ++i) if (SimGateway::uartCaptureByte(i) != want[i]) return false;
    return true;
}

static void begin() {
    SimGateway::calResetForTest();
    SimGateway::resetUartCapture();
    SimGateway::resetCdcCapture();
}

// ── A valid frame is consumed ─────────────────────────────────────────────────
static void testValidFrameConsumed() {
    uint8_t f[CAL_MAX_FRAME];
    const uint16_t n = OpenSkyhawk::calBuildFrame(f, sizeof(f), OpenSkyhawk::CAL_T_HELLO,
                                                  0x11, nullptr, 0);
    checkEq(F("[A1] HELLO frame is 10 bytes"), n, CAL_ENVELOPE_BYTES);

    begin();
    feed(f, n);
    checkEq(F("[A2] valid frame reaches the UART zero times"),
            SimGateway::uartCaptureCount(), 0);
    check  (F("[A3] valid frame produced a reply"), SimGateway::cdcCaptureCount() > 0);
}

// ── LEN must match TYPE, checked before the payload is buffered ───────────────
static void testLenGate() {
    // A HELLO carrying a non-zero LEN is not a frame at all.
    uint8_t bad[CAL_ENVELOPE_BYTES] = {
        0xAA, 0x53, 0x4B, 0x43, OpenSkyhawk::CAL_T_HELLO, 0x22, 0x05, 0x00, 0x00, 0x00
    };
    begin();
    feed(bad, 8);   // only through the header — the gate must fire here
    checkEq(F("[B1] wrong LEN flushes all 8 header bytes"), SimGateway::uartCaptureCount(), 8);
    check  (F("[B2] flushed bytes are byte-identical"), uartIs(bad, 8));
    checkEq(F("[B3] no reply emitted for a non-frame"), SimGateway::cdcCaptureCount(), 0);

    // The stall case the LEN rule exists to prevent: a plausible type with a noise length.
    uint8_t huge[8] = {
        0xAA, 0x53, 0x4B, 0x43, OpenSkyhawk::CAL_T_CAL_DATA, 0x00, 0xFF, 0xFF
    };
    begin();
    feed(huge, 8);
    checkEq(F("[B4] LEN 65535 rejected immediately, not buffered"),
            SimGateway::uartCaptureCount(), 8);

    // Unknown type — rejected at the same gate, since protocol versions must match.
    uint8_t unk[8] = { 0xAA, 0x53, 0x4B, 0x43, 0x7E, 0x00, 0x00, 0x00 };
    begin();
    feed(unk, 8);
    checkEq(F("[B5] unknown TYPE rejected at the length gate"),
            SimGateway::uartCaptureCount(), 8);

    // COMMIT carries exactly one axis, so it is fixed-length like everything else — there
    // are no variable-length types left, and no exception in the rule.
    uint8_t commitOk[8]  = { 0xAA, 0x53, 0x4B, 0x43, OpenSkyhawk::CAL_T_COMMIT, 0,  9, 0 };
    uint8_t commitBad[8] = { 0xAA, 0x53, 0x4B, 0x43, OpenSkyhawk::CAL_T_COMMIT, 0, 19, 0 };
    begin(); feed(commitOk, 8);
    checkEq(F("[B6] COMMIT LEN 9 accepted"), SimGateway::uartCaptureCount(), 0);
    begin(); feed(commitBad, 8);
    checkEq(F("[B7] old batch LEN 19 now rejected"), SimGateway::uartCaptureCount(), 8);
}

// ── Bad CRC hands every byte back ─────────────────────────────────────────────
static void testCrcRejection() {
    uint8_t f[CAL_MAX_FRAME];
    const uint16_t n = OpenSkyhawk::calBuildFrame(f, sizeof(f), OpenSkyhawk::CAL_T_GET_CAL,
                                                  0x33, nullptr, 0);
    f[n - 1] ^= 0x01;   // corrupt the CRC

    begin();
    feed(f, n);
    checkEq(F("[C1] bad CRC flushes the whole candidate"), SimGateway::uartCaptureCount(), n);
    check  (F("[C2] flushed bytes are byte-identical and in order"), uartIs(f, n));
    checkEq(F("[C3] no reply emitted"), SimGateway::cdcCaptureCount(), 0);
}

// ── Magic handling ────────────────────────────────────────────────────────────
static void testMagic() {
    // Partial match then a mismatch: everything held plus the offender is relayed.
    const uint8_t partial[4] = { 0xAA, 0x53, 0x4B, 0x99 };
    begin();
    feed(partial, sizeof(partial));
    checkEq(F("[D1] partial magic + mismatch relays all 4"), SimGateway::uartCaptureCount(), 4);
    check  (F("[D2] relayed in order"), uartIs(partial, sizeof(partial)));

    // A mismatching byte that is itself 0xAA must start a NEW candidate rather than being
    // relayed — 0xAA appears only at index 0 of the magic, so one re-test is enough.
    const uint8_t restart[3] = { 0xAA, 0x53, 0xAA };
    begin();
    feed(restart, sizeof(restart));
    checkEq(F("[D3] trailing 0xAA is held, not relayed"), SimGateway::uartCaptureCount(), 2);
    // ...and the held 0xAA lets a real frame complete straight afterwards.
    uint8_t f[CAL_MAX_FRAME];
    const uint16_t n = OpenSkyhawk::calBuildFrame(f, sizeof(f), OpenSkyhawk::CAL_T_HELLO,
                                                  0x44, nullptr, 0);
    feed(f + 1, n - 1);   // the leading 0xAA is already held
    checkEq(F("[D4] frame completes on the re-held 0xAA"), SimGateway::uartCaptureCount(), 2);
    check  (F("[D5] and it was answered"), SimGateway::cdcCaptureCount() > 0);

    // A run of 0xAA then a real frame — the naive flush rule must not lose the match.
    begin();
    const uint8_t run[3] = { 0xAA, 0xAA, 0xAA };
    feed(run, sizeof(run));
    feed(f + 1, n - 1);
    check(F("[D6] 0xAA run then a frame still parses"), SimGateway::cdcCaptureCount() > 0);
}

// ── Ordinary traffic is untouched ─────────────────────────────────────────────
static void testPassthrough() {
    const char* msg = "ARM_MASTER 1\n";
    const size_t n = strlen(msg);
    begin();
    feed((const uint8_t*)msg, n);
    checkEq(F("[E1] DCS-BIOS text relayed in full"), SimGateway::uartCaptureCount(), n);
    check  (F("[E2] byte-identical"), uartIs((const uint8_t*)msg, n));
    checkEq(F("[E3] nothing sent back"), SimGateway::cdcCaptureCount(), 0);

    // The DCS-BIOS export stream's own sync header must survive untouched.
    const uint8_t sync[6] = { 0x55, 0x55, 0x55, 0x55, 0x10, 0x08 };
    begin();
    feed(sync, sizeof(sync));
    check(F("[E4] 0x55 0x55 export sync passes through"), uartIs(sync, sizeof(sync)));
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println(F("=== SimGateway calibration framing test ==="));
    testValidFrameConsumed();
    testLenGate();
    testCrcRejection();
    testMagic();
    testPassthrough();
    Serial.println(g_allPass ? F("=== ALL PASS ===") : F("=== FAILURES PRESENT ==="));
}

void loop() {}
