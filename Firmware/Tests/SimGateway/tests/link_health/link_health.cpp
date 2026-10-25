// SimGateway - link health: recovery ladder, FAULT self-clear, counters, GET_DIAG (#355)
//
// Everything here is decision logic driven with injected time and inputs: no USB detach, no
// reboot, no UART restart happens in a SIMGATEWAY_TEST build. The tests assert what
// production would do.
//
// Covers:
//   - healthy link: the ladder never acts
//   - unmounted 5 s → DETACH; ATTACH 100 ms later even if the link looks healthy meanwhile
//   - still unhealthy 5 s after re-attach → REBOOT
//   - health at any point resets the ladder; the next failure starts from the bottom
//   - after a recovery reboot: re-attach every 60 s, never REBOOT again
//   - suspended alone (host asleep) is healthy; suspended with a recent UART fault is not
//   - USB-loss times stamp on the healthy → unhealthy edge only, never at boot
//   - FAULT latched 3 s → self-clear (production restarts the UART); errors re-latch it
//   - UART error bits counted separately, first/last times recorded
//   - CDC stall: idle / no host / draining is never a stall; full + no drain for CDC_STALL_MS is
//   - mounted, not suspended, continuous traffic, no CDC capacity → ladder DETACHes (review #358)
//   - GET_DIAG answered outside a session, SEQ echoed, payload matching diagnostics()
//
// Flash:
//   pio run -e test_link_health -t upload
// Monitor: open USB CDC (115200) on the Pico.

#include <Arduino.h>
#include <SimGateway.h>

namespace OS = OpenSkyhawk;
using OS::LinkAction;

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

static LinkAction step(uint32_t now, bool mounted, bool suspended = false) {
    return SimGateway::linkStepForTest(now, mounted, suspended);
}

static void fresh(bool afterRecoveryReboot = false) {
    SimGateway::diagResetForTest();
    SimGateway::linkResetForTest(afterRecoveryReboot);
    SimGateway::statusResetForTest();
}

// Run the ladder unhealthy from `from` to `to` (inclusive, 10 ms ticks) and report the first
// non-NONE action and when it happened.
static LinkAction firstAction(uint32_t from, uint32_t to, uint32_t& at, bool mounted = false,
                              bool suspended = false) {
    for (uint32_t t = from; t <= to; t += 10) {
        const LinkAction a = step(t, mounted, suspended);
        if (a != LinkAction::NONE) { at = t; return a; }
    }
    at = 0;
    return LinkAction::NONE;
}

// ── The ladder ────────────────────────────────────────────────────────────────
static void testLadder() {
    uint32_t at = 0;

    fresh();
    bool quiet = true;
    for (uint32_t t = 0; t < 120000; t += 50) quiet &= (step(t, true) == LinkAction::NONE);
    check(F("[A1] healthy for 2 min: no action"), quiet);

    fresh();
    step(1000, true);                                 // healthy first
    LinkAction a = firstAction(2000, 20000, at);
    check(F("[A2] unmounted: first action is DETACH"), a == LinkAction::DETACH);
    checkEq(F("[A3] after exactly LINK_BAD_MS"), at, 2000 + OS::LINK_BAD_MS);
    checkEq(F("[A4] counted as a re-attach"), SimGateway::diagnostics().usbReattaches, 1);

    const uint32_t det = at;
    check(F("[A5] no ATTACH before the gap"), step(det + OS::LINK_REATTACH_GAP_MS - 1, true) == LinkAction::NONE);
    check(F("[A6] ATTACH after the gap, even though 'healthy' was sampled while detached"),
          step(det + OS::LINK_REATTACH_GAP_MS, true) == LinkAction::ATTACH);

    const uint32_t att = det + OS::LINK_REATTACH_GAP_MS;
    a = firstAction(att + 10, att + 20000, at);
    check(F("[A7] still unhealthy after re-attach: REBOOT"), a == LinkAction::REBOOT);
    checkEq(F("[A8] after LINK_REBOOT_AFTER_MS"), at, att + OS::LINK_REBOOT_AFTER_MS);

    // Recovery after the re-attach resets the ladder from the bottom.
    fresh();
    step(0, true);
    firstAction(10, 20000, at);                       // DETACH
    step(at + OS::LINK_REATTACH_GAP_MS, false);       // ATTACH
    check(F("[A9] link comes back after re-attach: NONE"), step(at + 500, true) == LinkAction::NONE);
    const uint32_t back = at + 500;
    a = firstAction(back + 10, back + 30000, at);
    check(F("[A10] next failure starts at DETACH, not REBOOT"), a == LinkAction::DETACH);
    checkEq(F("[A11] again after LINK_BAD_MS"), at, back + 10 + OS::LINK_BAD_MS);
}

// ── Backoff after a recovery reboot ───────────────────────────────────────────
static void testBackoff() {
    uint32_t at = 0;
    fresh(/*afterRecoveryReboot*/ true);

    LinkAction a = firstAction(0, 120000, at);
    check(F("[B1] after a recovery reboot: first action is DETACH"), a == LinkAction::DETACH);
    checkEq(F("[B2] after LINK_BACKOFF_MS, not LINK_BAD_MS"), at, OS::LINK_BACKOFF_MS);
    check(F("[B3] ATTACH follows"), step(at + OS::LINK_REATTACH_GAP_MS, false) == LinkAction::ATTACH);

    const uint32_t att = at + OS::LINK_REATTACH_GAP_MS;
    a = firstAction(att + 10, att + 200000, at);
    check(F("[B4] never REBOOT again in backoff"), a == LinkAction::DETACH);
    check(F("[B5] next re-attach a backoff period later"),
          at >= att + OS::LINK_REBOOT_AFTER_MS + OS::LINK_BACKOFF_MS);

    // Health clears the backoff: the following failure gets the fast ladder again.
    step(at + OS::LINK_REATTACH_GAP_MS, false);       // ATTACH
    const uint32_t ok = at + 1000;
    step(ok, true);
    a = firstAction(ok + 10, ok + 30000, at);
    check(F("[B6] healthy once → backoff cleared, DETACH after LINK_BAD_MS"),
          a == LinkAction::DETACH && at == ok + 10 + OS::LINK_BAD_MS);
}

// ── Health definition and loss stamps ─────────────────────────────────────────
static void testHealth() {
    uint32_t at = 0;

    fresh();
    bool quiet = true;
    for (uint32_t t = 0; t < 120000; t += 50) quiet &= (step(t, true, /*suspended*/ true) == LinkAction::NONE);
    check(F("[C1] suspended with no UART fault (host asleep): no action"), quiet);
    checkEq(F("[C2] and no USB loss recorded"), SimGateway::diagnostics().firstUsbLossMs, 0);

    fresh();
    step(1000, true);
    SimGateway::diagUartErrorsForTest(1500, false, true, false, false);   // FE
    LinkAction a = firstAction(2000, 20000, at, /*mounted*/ true, /*suspended*/ true);
    check(F("[C3] suspended + recent UART fault: DETACH"), a == LinkAction::DETACH);
    checkEq(F("[C4] loss stamped on the edge"), SimGateway::diagnostics().firstUsbLossMs, 2000);

    fresh();
    step(1000, true);
    SimGateway::diagUartErrorsForTest(1500, false, true, false, false);
    a = firstAction(1500 + 30000, 1500 + 60000, at, true, true);
    check(F("[C5] fault older than 30 s: suspended is healthy again"), a == LinkAction::NONE);

    // Boot: never mounted is unhealthy for the ladder, but it is not a "loss".
    fresh();
    step(0, false);
    step(500, false);
    checkEq(F("[C6] not-yet-enumerated is not a USB loss"), SimGateway::diagnostics().firstUsbLossMs, 0);
    step(800, true);
    step(9000, false);
    step(9500, true);
    step(12000, false);
    checkEq(F("[C7] first loss time"), SimGateway::diagnostics().firstUsbLossMs, 9000);
    checkEq(F("[C8] last loss time"),  SimGateway::diagnostics().lastUsbLossMs,  12000);
}

// ── FAULT self-clear ──────────────────────────────────────────────────────────
static void testFaultRestart() {
    fresh();
    check(F("[D1] FAULT latches on an error"), SimGateway::statusFaultStep(1000, true, false));
    check(F("[D2] not cleared before FAULT_RESTART_MS"), !SimGateway::statusFaultRestartCheck(3999));
    check(F("[D3] self-clear at 3 s on a silent bus"), SimGateway::statusFaultRestartCheck(4000));
    check(F("[D4] latch is clear afterwards"), !SimGateway::statusFaultStep(4010, false, false));
    checkEq(F("[D5] counted as a UART restart"), SimGateway::diagnostics().uartRestarts, 1);
    check(F("[D6] nothing more to clear"), !SimGateway::statusFaultRestartCheck(9000));

    // A link that keeps erroring re-latches at once and restarts every 3 s.
    fresh();
    SimGateway::statusFaultStep(1000, true, false);
    SimGateway::statusFaultStep(2000, true, false);   // re-stamp does not move the restart point
    check(F("[D7] restart measured from the first error, not the latest"),
          SimGateway::statusFaultRestartCheck(4000));
    check(F("[D8] persistent errors re-latch immediately"), SimGateway::statusFaultStep(4010, true, false));
    check(F("[D9] and restart again 3 s later"), SimGateway::statusFaultRestartCheck(7010));
}

// ── UART error counters ───────────────────────────────────────────────────────
static void testUartCounters() {
    fresh();
    SimGateway::diagUartErrorsForTest(0, false, false, false, false);
    checkEq(F("[E1] a clean tick counts nothing"), SimGateway::diagnostics().firstUartErrMs, 0);

    SimGateway::diagUartErrorsForTest(0, true, false, false, false);
    checkEq(F("[E2] error at millis 0 is still recorded (0 means never)"),
            SimGateway::diagnostics().firstUartErrMs, 1);
    SimGateway::diagUartErrorsForTest(500, false, true, true, false);
    SimGateway::diagUartErrorsForTest(700, false, true, false, true);
    const SimGateway::GatewayDiag& d = SimGateway::diagnostics();
    checkEq(F("[E3] overrun"), d.uartOverrun, 1);
    checkEq(F("[E4] framing"), d.uartFraming, 2);
    checkEq(F("[E5] break"),   d.uartBreak,   1);
    checkEq(F("[E6] parity"),  d.uartParity,  1);
    checkEq(F("[E7] first error time kept"), d.firstUartErrMs, 1);
    checkEq(F("[E8] last error time"),       d.lastUartErrMs,  700);

    for (uint32_t i = 0; i < 70000; ++i) SimGateway::diagUartErrorsForTest(1000, false, true, false, false);
    checkEq(F("[E9] counters saturate, never wrap"), d.uartFraming, 0xFFFF);
}

// ── CDC stall (configured but not draining) ───────────────────────────────────
static constexpr uint16_t FIFO = 256;

static void testCdcStall() {
    fresh();
    bool ok = true;
    for (uint32_t t = 0; t <= 10000; t += 10) ok &= !SimGateway::cdcStallStepForTest(t, true, FIFO, FIFO, 0);
    check(F("[G1] DTR held, nothing pending: never a stall"), ok);

    fresh();
    ok = true;
    for (uint32_t t = 0; t <= 10000; t += 10) ok &= !SimGateway::cdcStallStepForTest(t, false, 0, FIFO, 0);
    check(F("[G2] FIFO full but no host (DTR low): never a stall"), ok);

    // Draining exactly as fast as we refill: room never grows, but `written` shows the drain.
    fresh();
    ok = true;
    for (uint32_t t = 0; t <= 10000; t += 10) ok &= !SimGateway::cdcStallStepForTest(t, true, 40, FIFO, 64);
    check(F("[G3] constant room but bytes written each tick: progress, not a stall"), ok);

    fresh();
    SimGateway::cdcStallStepForTest(0, true, 10, FIFO, 0);
    check(F("[G4] full, no drain: not yet at CDC_STALL_MS - 10"),
          !SimGateway::cdcStallStepForTest(SimGateway::CDC_STALL_MS - 10, true, 10, FIFO, 0));
    check(F("[G5] stalled at CDC_STALL_MS"),
          SimGateway::cdcStallStepForTest(SimGateway::CDC_STALL_MS, true, 10, FIFO, 0));
    SimGateway::cdcStallStepForTest(SimGateway::CDC_STALL_MS + 500, true, 10, FIFO, 0);
    checkEq(F("[G6] one stall counted per onset"), SimGateway::diagnostics().cdcStalls, 1);
    check(F("[G7] a drain clears it"),
          !SimGateway::cdcStallStepForTest(SimGateway::CDC_STALL_MS + 600, true, 74, FIFO, 0));

    // The review's case: mounted, not suspended, PanelBridge lines keep arriving, CDC has no
    // room for any of them. Before the fix this was healthy forever.
    fresh();
    SimGateway::resetParser();
    SimGateway::resetCdcCapture();
    SimGateway::cdcSetRoomForTest(0);
    step(0, true);                                     // healthy first
    LinkAction a = LinkAction::NONE;
    uint32_t at = 0;
    for (uint32_t t = 10; t <= 20000 && a == LinkAction::NONE; t += 10) {
        for (const char* c = "GEAR 1\n"; *c; ++c) SimGateway::feedByte((uint8_t)*c);
        const bool stalled = SimGateway::cdcStallStepForTest(t, true, 0, FIFO, 0);
        a = SimGateway::linkStepForTest(t, /*mounted*/ true, /*suspended*/ false, stalled);
        if (a != LinkAction::NONE) at = t;
    }
    SimGateway::cdcSetRoomForTest(-1);
    check(F("[G8] mounted + stalled CDC: ladder DETACHes"), a == LinkAction::DETACH);
    check(F("[G9] after CDC_STALL_MS + LINK_BAD_MS"),
          at >= SimGateway::CDC_STALL_MS + OS::LINK_BAD_MS && at <= SimGateway::CDC_STALL_MS + OS::LINK_BAD_MS + 20);
    check(F("[G10] the lines were dropped, not written"),
          SimGateway::cdcCaptureCount() == 0 && SimGateway::diagnostics().linesDropped > 0);
    checkEq(F("[G11] stall counted"), SimGateway::diagnostics().cdcStalls, 1);
}

// ── GET_DIAG ──────────────────────────────────────────────────────────────────
static uint16_t rx16(size_t i) {
    return (uint16_t)(SimGateway::cdcCaptureByte(i) | ((uint16_t)SimGateway::cdcCaptureByte(i + 1) << 8));
}
static uint32_t rx32(size_t i) { return (uint32_t)rx16(i) | ((uint32_t)rx16(i + 2) << 16); }

static void testGetDiag() {
    fresh();
    SimGateway::calResetForTest();
    SimGateway::cdcSetRoomForTest(-1);
    SimGateway::diagUartErrorsForTest(1234, false, false, true, false);   // BE
    step(100, true);
    step(5678, false);                                                    // USB loss

    uint8_t f[OS::CAL_MAX_FRAME];
    const uint16_t n = OS::calBuildFrame(f, sizeof(f), OS::CAL_T_GET_DIAG, 0x5A, nullptr, 0);
    SimGateway::resetCdcCapture();
    SimGateway::resetUartCapture();
    for (uint16_t i = 0; i < n; ++i) SimGateway::feedCdcByte(f[i]);

    checkEq(F("[F1] nothing relayed to PanelBridge"), SimGateway::uartCaptureCount(), 0);
    checkEq(F("[F2] DIAG frame length"), SimGateway::cdcCaptureCount(), OS::CAL_ENVELOPE_BYTES + OS::CAL_DIAG_BYTES);
    checkEq(F("[F3] type DIAG, outside a session"), SimGateway::cdcCaptureByte(4), OS::CAL_T_DIAG);
    checkEq(F("[F4] SEQ echoed"), SimGateway::cdcCaptureByte(5), 0x5A);
    check(F("[F5] reply CRC valid"), OS::calFrameCrcOk(
          [] { static uint8_t b[OS::CAL_MAX_FRAME];
               for (size_t i = 0; i < SimGateway::cdcCaptureCount(); ++i) b[i] = SimGateway::cdcCaptureByte(i);
               return b; }(), (uint16_t)SimGateway::cdcCaptureCount()));

    const size_t P = 8;   // payload starts after the 8-byte header
    const SimGateway::GatewayDiag& d = SimGateway::diagnostics();
    checkEq(F("[F6] first UART error time"), rx32(P + OS::CAL_DIAG_FIRST_UART_ERR_MS), 1234);
    checkEq(F("[F7] last UART error time"),  rx32(P + OS::CAL_DIAG_LAST_UART_ERR_MS),  1234);
    checkEq(F("[F8] first USB loss time"),   rx32(P + OS::CAL_DIAG_FIRST_USB_LOSS_MS), 5678);
    checkEq(F("[F9] break counter"),         rx16(P + OS::CAL_DIAG_UART_BREAK),        1);
    checkEq(F("[F10] framing counter"),      rx16(P + OS::CAL_DIAG_UART_FRAMING),      0);
    checkEq(F("[F11] reset reason byte"),
            SimGateway::cdcCaptureByte(P + OS::CAL_DIAG_RESET_REASON), d.resetReason);
    check(F("[F12] uptime is a live millis()"), rx32(P + OS::CAL_DIAG_UPTIME_MS) > 0);
}

static void runTests() {
    testLadder();
    testBackoff();
    testHealth();
    testFaultRestart();
    testUartCounters();
    testCdcStall();
    testGetDiag();

    Serial.println();
    Serial.println(g_allPass ? F("=== RESULT: PASS ===") : F("=== RESULT: FAIL ==="));
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println(F("=== link_health test ==="));
    runTests();
}

void loop() {}
