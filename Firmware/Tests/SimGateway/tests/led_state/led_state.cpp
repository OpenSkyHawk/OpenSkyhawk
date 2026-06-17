// SimGateway - status-LED state machine test
//
// Exercises the SimGateway status-LED state machine (issue #94) via the
// SIMGATEWAY_TEST hooks — no GPIO, TinyUSB, or PL011 register access. The pure
// state-selection + animation logic takes `now` as a parameter, so the whole
// machine is deterministic under injected inputs.
//
// Covers:
//   - state -> (colour, animation, pin levels)
//   - priority precedence (FAULT over everything; everMounted -> NO_HOST)
//   - STREAMING -> USB_IDLE 500 ms decay boundary
//   - INIT -> NO_HOST init-window boundary (2000 ms)
//   - FAULT latch / recovery: min-hold, clean-data-resumes, silent-bus holds,
//     sustained re-stamp, stale-pre-fault byte does not clear
//   - animation phase timing (SLOW 1 Hz, FAST 4 Hz, SOLID)
//
// Flash:
//   pio run -e test_led_state -t upload
// Monitor: open USB CDC (115200) on the Pico.

#include <Arduino.h>
#include <SimGateway.h>

using SimGateway::Anim;
using SimGateway::LedState;

static bool g_allPass = true;

static void check(const __FlashStringHelper* label, bool cond) {
    Serial.print(label);
    Serial.println(cond ? F(": PASS") : F(": FAIL"));
    g_allPass &= cond;
}

// Inject inputs and resolve. Caller then reads statusState/Anim/Red/Green.
static void resolve(uint32_t now, bool mounted, uint32_t lastCdcRxMs, bool faultActive) {
    SimGateway::statusInject(now, mounted, lastCdcRxMs, faultActive);
    SimGateway::statusResolve();
}

// ── State -> (colour, animation, pin levels) ──────────────────────────────────
static void testStateMapping() {
    // FAULT: RED FAST. now=0 -> FAST phase on -> red on, green off.
    SimGateway::statusResetForTest();
    resolve(/*now*/0, /*mounted*/true, /*lastCdcRxMs*/0, /*fault*/true);
    check(F("[A1] FAULT = RED FAST, green off"),
          SimGateway::statusState() == LedState::FAULT &&
          SimGateway::statusAnim()  == Anim::FAST &&
          SimGateway::statusRedLevel() && !SimGateway::statusGreenLevel());

    // NO_HOST (never mounted, init window expired): RED SOLID.
    SimGateway::statusResetForTest();
    resolve(/*now*/3000, /*mounted*/false, 0, false);
    check(F("[A2] NO_HOST (init expired) = RED SOLID"),
          SimGateway::statusState() == LedState::NO_HOST &&
          SimGateway::statusAnim()  == Anim::SOLID &&
          SimGateway::statusRedLevel() && !SimGateway::statusGreenLevel());

    // NO_HOST via unplug after a mount (now < init window, but everMounted).
    SimGateway::statusResetForTest();
    SimGateway::statusInject(100, /*mounted*/true, 0, false); // sticks everMounted
    resolve(/*now*/200, /*mounted*/false, 0, false);
    check(F("[A3] unplug after mount = NO_HOST"),
          SimGateway::statusState() == LedState::NO_HOST);

    // STREAMING: GREEN SOLID.
    SimGateway::statusResetForTest();
    resolve(/*now*/1000, /*mounted*/true, /*lastCdcRxMs*/1000, false);
    check(F("[A4] STREAMING = GREEN SOLID, red off"),
          SimGateway::statusState() == LedState::STREAMING &&
          SimGateway::statusAnim()  == Anim::SOLID &&
          SimGateway::statusGreenLevel() && !SimGateway::statusRedLevel());

    // USB_IDLE: GREEN SLOW. now=2000 -> SLOW phase on.
    SimGateway::statusResetForTest();
    resolve(/*now*/2000, /*mounted*/true, /*lastCdcRxMs*/1000, false);
    check(F("[A5] USB_IDLE = GREEN SLOW, red off"),
          SimGateway::statusState() == LedState::USB_IDLE &&
          SimGateway::statusAnim()  == Anim::SLOW &&
          SimGateway::statusGreenLevel() && !SimGateway::statusRedLevel());

    // INIT: RED SLOW. now=100 -> SLOW phase on.
    SimGateway::statusResetForTest();
    resolve(/*now*/100, /*mounted*/false, 0, false);
    check(F("[A6] INIT = RED SLOW, green off"),
          SimGateway::statusState() == LedState::INIT &&
          SimGateway::statusAnim()  == Anim::SLOW &&
          SimGateway::statusRedLevel() && !SimGateway::statusGreenLevel());

    // Priority: FAULT overrides NO_HOST (unmounted + fault -> FAULT).
    SimGateway::statusResetForTest();
    resolve(/*now*/3000, /*mounted*/false, 0, /*fault*/true);
    check(F("[A7] FAULT overrides NO_HOST"),
          SimGateway::statusState() == LedState::FAULT);
}

// ── STREAMING -> USB_IDLE 500 ms decay boundary ───────────────────────────────
static void testStreamDecay() {
    SimGateway::statusResetForTest();
    resolve(1499, true, 1000, false); // 499 ms since data
    bool a = SimGateway::statusState() == LedState::STREAMING;
    resolve(1500, true, 1000, false); // 500 ms — inclusive
    bool b = SimGateway::statusState() == LedState::STREAMING;
    resolve(1501, true, 1000, false); // 501 ms — decayed
    bool c = SimGateway::statusState() == LedState::USB_IDLE;
    check(F("[B1] STREAMING<=500ms, USB_IDLE>500ms"), a && b && c);
}

// ── INIT -> NO_HOST init-window boundary (2000 ms) ────────────────────────────
static void testInitWindow() {
    SimGateway::statusResetForTest();
    resolve(1999, false, 0, false);
    bool a = SimGateway::statusState() == LedState::INIT;
    resolve(2000, false, 0, false);
    bool b = SimGateway::statusState() == LedState::NO_HOST;
    check(F("[C1] INIT<2000ms, NO_HOST>=2000ms"), a && b);
}

// ── FAULT latch / recovery ────────────────────────────────────────────────────
static void testFaultLatch() {
    // (a) error latches; (b) clean RX before min-hold keeps it; (c) clean RX after
    // min-hold clears it.
    SimGateway::statusResetForTest();
    bool latched = SimGateway::statusFaultStep(/*now*/0, /*rsrError*/true, /*uartRxMoved*/false);
    bool held    = SimGateway::statusFaultStep(/*now*/1000, false, /*clean rx*/true);
    bool cleared = !SimGateway::statusFaultStep(/*now*/2000, false, /*clean rx*/true);
    check(F("[D1] latch, hold<2s, clear>=2s on clean data"), latched && held && cleared);

    // (d) silent bus: no clean RX after the fault -> never clears.
    SimGateway::statusResetForTest();
    SimGateway::statusFaultStep(0, true, false);
    bool stillLatched = SimGateway::statusFaultStep(5000, false, /*no rx*/false);
    check(F("[D2] silent bus holds FAULT"), stillLatched);

    // (e) sustained errors re-stamp -> hold extends past the original window.
    SimGateway::statusResetForTest();
    SimGateway::statusFaultStep(0, true, false);
    SimGateway::statusFaultStep(250, true, false);            // re-stamp at 250
    bool reHeld = SimGateway::statusFaultStep(2000, false, true); // 2000-250 < 2000
    check(F("[D3] sustained re-stamp keeps FAULT"), reHeld);

    // (f) an error-free byte from BEFORE the fault must not clear it.
    SimGateway::statusResetForTest();
    SimGateway::statusFaultStep(0, false, /*clean rx pre-fault*/true); // lastUartRx=0
    SimGateway::statusFaultStep(100, true, false);                     // fault at 100
    bool stalePre = SimGateway::statusFaultStep(2200, false, false);   // no rx since fault
    check(F("[D4] stale pre-fault byte does not clear"), stalePre);
}

// ── Animation phase timing ────────────────────────────────────────────────────
static void testPhaseTiming() {
    // FAST (4 Hz, 250 ms): on [0,125), off [125,250). Use FAULT (RED FAST).
    SimGateway::statusResetForTest();
    auto fastRedAt = [](uint32_t now) {
        resolve(now, true, now, /*fault*/true);
        return SimGateway::statusRedLevel();
    };
    check(F("[E1] FAST on@0,124 off@125,249 on@250"),
          fastRedAt(0) && fastRedAt(124) && !fastRedAt(125) && !fastRedAt(249) && fastRedAt(250));

    // SLOW (1 Hz, 1000 ms): on [0,500), off [500,1000). Use INIT (RED SLOW),
    // which needs unmounted + now < 2000 + never mounted.
    SimGateway::statusResetForTest();
    auto slowRedAt = [](uint32_t now) {
        resolve(now, false, 0, false);
        return SimGateway::statusRedLevel();
    };
    check(F("[E2] SLOW on@0,499 off@500,999 on@1000"),
          slowRedAt(0) && slowRedAt(499) && !slowRedAt(500) && !slowRedAt(999) && slowRedAt(1000));

    // SOLID: on at all `now`. Use NO_HOST (RED SOLID).
    SimGateway::statusResetForTest();
    auto solidRedAt = [](uint32_t now) {
        resolve(now, false, 0, false);
        return SimGateway::statusRedLevel();
    };
    check(F("[E3] SOLID on at all now"),
          solidRedAt(2000) && solidRedAt(2137) && solidRedAt(9999));
}

static void runTests() {
    g_allPass = true;
    testStateMapping();
    testStreamDecay();
    testInitWindow();
    testFaultLatch();
    testPhaseTiming();
    Serial.println();
    Serial.println(g_allPass ? F("=== RESULT: PASS ===") : F("=== RESULT: FAIL ==="));
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println(F("=== led_state test ==="));
    runTests();
}

void loop() {}
