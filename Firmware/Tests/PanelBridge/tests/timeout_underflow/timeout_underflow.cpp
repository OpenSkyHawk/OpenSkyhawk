// PanelBridge — node-timeout underflow guard (#225)
//
// Regression test for the false split-second "offline" flicker. checkNodeTimeouts computed
// `now - lastSeenMs` UNSIGNED, so when `now` was sampled a millisecond BEHIND lastSeenMs — a tick
// landing between the loop's millis() and onCanRx stamping lastSeenMs during drain() — the
// difference wrapped to ~4.29e9 > HB_TIMEOUT_MS and declared a live node dead for one loop. The fix
// samples `now` after drain() AND compares signed. This drives testCheckTimeouts() with now behind
// lastSeenMs and asserts no false dead, then with a genuinely overdue now and asserts a real timeout
// still fires. Pure logic on a bare STM32F103CB — no CAN bus.
//
// Pass: every line [PASS]; prints "=== ALL PASS ===".

#define DCSBIOS_DEFAULT_SERIAL
#include <DcsBios.h>
#include <PanelBridge.h>
#include <STM32Board.h>

static volatile bool    deadFired = false;
static volatile uint8_t deadNode  = 0;
static void onDead(uint8_t n) { deadFired = true; deadNode = n; }

static bool pass = true;
static void check(const char* label, bool ok) {
    if (!ok) pass = false;
    auto& d = STM32Board::diagSerial();
    d.print(ok ? F("[PASS] ") : F("[FAIL] "));
    d.println(label);
}

void setup() {
    STM32Board::setDebug(true);
    PanelBridge::onNodeDead(onDead);
    PanelBridge::setup();
    DcsBios::setup();
    auto& d = STM32Board::diagSerial();
    d.println(F("=== PanelBridge timeout_underflow (#225) ==="));

    // Node 1 alive; lastSeenMs = millis() (some value M > 1).
    PanelBridge::testFeedHeartbeat(1, 0x00, 10, 0, 0x0000);

    // Case A — the #225 underflow: `now` sampled BEHIND lastSeenMs. Must NOT declare dead.
    deadFired = false;
    PanelBridge::testCheckTimeouts(1);                  // now = 1 ms << lastSeenMs
    check("now < lastSeenMs -> no false dead", !deadFired);

    // Case B — a genuinely overdue node still times out (HB_TIMEOUT_MS = 3000).
    deadFired = false;
    PanelBridge::testCheckTimeouts(millis() + 4000);    // now > lastSeenMs + HB_TIMEOUT_MS
    check("overdue node -> real timeout fires", deadFired && deadNode == 1);

    d.println(pass ? F("=== ALL PASS ===") : F("=== FAIL ==="));
}

void loop() {}
