// ActionButton — press-edge semantics test
//
// Verifies the core contract:
//   press          → exactly one EVT_ACTION, value 0 (TOGGLE)
//   held           → still exactly one, however many poll() calls elapse
//   release        → nothing emitted
//   press again    → fires again (release re-armed it)
//
// The "held" case is the one that matters most: the guard is structural (no state change ⇒ no
// emit), not a repeat-rate limit, so there is no interval at which it could begin repeating.
//
// Hardware: STM32. PB0→PA0 jumper wire required.
// PB0: output — drives button state. PA0: input — ActionButton reads this.

#include <Arduino.h>
#include <STM32Board.h>
#include <Inputs/ActionButton/ActionButton.h>

static constexpr uint16_t CTRL_ID  = 0xABCD;
static constexpr uint8_t  PIN_CTRL = PB0;   // drives button state
static constexpr uint8_t  PIN_BTN  = PA0;   // ActionButton input

static uint8_t  gEvtCount = 0;
static uint16_t gLastVal  = 0xFFFF;

static void onCan(uint32_t canId, const uint8_t* data, uint8_t len) {
    if (canId != canIdEvtAction(NODE_ID) || len < 8) return;
    const ControlPacketPair* pair = reinterpret_cast<const ControlPacketPair*>(data);
    if (pair->a.controlId == CTRL_ID)      { gEvtCount++; gLastVal = pair->a.value; }
    else if (pair->b.controlId == CTRL_ID) { gEvtCount++; gLastVal = pair->b.value; }
}

static void flushDrain() {
    CANProtocol::flushBatched(canIdEvtAction(NODE_ID));
    delay(2);
    CANProtocol::drain();
}

/** @brief Drive the button and give the class a poll, honouring the debounce window. */
static void setButton(bool pressed) {
    digitalWrite(PIN_CTRL, pressed ? LOW : HIGH);   // active-LOW
    delayMicroseconds(100);
}

OpenSkyhawk::ActionButton gBtn(CTRL_ID, PinRef(PIN_BTN));

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== ActionButton press_edge ===");
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

    // ── Baseline: released, then forceReport() to initialise ─────────────────
    setButton(false);
    gBtn.forceReport();
    flushDrain();
    check("forceReport() emits nothing", gEvtCount == 0);

    // ── Press → exactly one TOGGLE ───────────────────────────────────────────
    setButton(true);
    gBtn.poll();
    flushDrain();
    check("press: 1 EVT", gEvtCount == 1);
    check("press: value 0 (TOGGLE)", gLastVal == 0);

    // ── Held → still exactly one, across many polls and past the debounce ────
    delay(OpenSkyhawk::ActionButton::DEBOUNCE_MS * 3);
    for (int i = 0; i < 50; i++) { gBtn.poll(); delay(1); }
    flushDrain();
    check("held for 50 polls / 60+ ms: still 1 EVT", gEvtCount == 1);

    // ── Release → nothing ────────────────────────────────────────────────────
    setButton(false);
    gBtn.poll();
    flushDrain();
    check("release: still 1 EVT", gEvtCount == 1);

    // ── Press again → re-armed, fires once more ──────────────────────────────
    delay(OpenSkyhawk::ActionButton::DEBOUNCE_MS * 2);
    setButton(true);
    gBtn.poll();
    flushDrain();
    check("second press: 2 EVTs", gEvtCount == 2);
    check("second press: value 0 (TOGGLE)", gLastVal == 0);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
