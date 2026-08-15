// ActionButton — contact-bounce suppression
//
// A mechanical contact produces several real HIGH→LOW transitions in a few milliseconds, and
// poll() runs every loop iteration, so each one is a genuine press edge as far as the code is
// concerned. Edge detection alone does not cover this.
//
// It matters more here than for a level-tracking switch: every TOGGLE flips sim state, so an even
// number of bounces cancels and an odd number flips. Without suppression the switch's final
// position would depend on how many times the contact happened to bounce.
//
// Verifies:
//   a burst of press/release transitions inside the debounce window → exactly one EVT
//   a clean press after the window has elapsed → fires normally (suppression is not sticky)
//
// Hardware: STM32. PB0→PA0 jumper wire required.

#include <Arduino.h>
#include <STM32Board.h>
#include <Inputs/ActionButton/ActionButton.h>

static constexpr uint16_t CTRL_ID  = 0xABCD;
static constexpr uint8_t  PIN_CTRL = PB0;
static constexpr uint8_t  PIN_BTN  = PA0;

static uint8_t gEvtCount = 0;

static void onCan(uint32_t canId, const uint8_t* data, uint8_t len) {
    if (canId != canIdEvtAction(NODE_ID) || len < 8) return;
    const ControlPacketPair* pair = reinterpret_cast<const ControlPacketPair*>(data);
    if (pair->a.controlId == CTRL_ID || pair->b.controlId == CTRL_ID) gEvtCount++;
}

static void flushDrain() {
    CANProtocol::flushBatched(canIdEvtAction(NODE_ID));
    delay(2);
    CANProtocol::drain();
}

OpenSkyhawk::ActionButton gBtn(CTRL_ID, PinRef(PIN_BTN));

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== ActionButton debounce ===");
    STM32Board::diagSerial().println("Hardware: PB0->PA0 jumper wire required.");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    pinMode(PIN_CTRL, OUTPUT);
    gBtn.configure();

    CANProtocol::onReceive(onCan);
    CANProtocol::filterAcceptId(canIdEvtAction(NODE_ID));
    CANProtocol::startLoopback();

    digitalWrite(PIN_CTRL, HIGH);         // released
    delayMicroseconds(100);
    gBtn.forceReport();
    flushDrain();

    // ── Bouncing press: 8 transitions inside the debounce window ─────────────
    // A real bouncing contact; each LOW is a true press edge to the code.
    for (int i = 0; i < 8; i++) {
        digitalWrite(PIN_CTRL, LOW);
        delayMicroseconds(200);
        gBtn.poll();
        digitalWrite(PIN_CTRL, HIGH);
        delayMicroseconds(200);
        gBtn.poll();
    }
    digitalWrite(PIN_CTRL, LOW);          // settles pressed
    delayMicroseconds(100);
    gBtn.poll();
    flushDrain();
    check("8-transition bounce: exactly 1 EVT", gEvtCount == 1);

    // ── Suppression is not sticky: a later clean press still fires ───────────
    digitalWrite(PIN_CTRL, HIGH);
    delayMicroseconds(100);
    delay(OpenSkyhawk::ActionButton::DEBOUNCE_MS * 3);
    gBtn.poll();                          // observe the release
    flushDrain();
    check("release after bounce: still 1 EVT", gEvtCount == 1);

    digitalWrite(PIN_CTRL, LOW);
    delayMicroseconds(100);
    gBtn.poll();
    flushDrain();
    check("clean press after window: 2 EVTs", gEvtCount == 2);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
