// ActionButton — reverse polarity
//
// Verifies the active edge inverts, and that readPressed() is the single interpretation used by
// both poll() and forceReport(). If the two paths ever disagreed, the boot-held seed would be the
// opposite of what poll() reads and the node would fire a spurious TOGGLE on its first iteration —
// so the boot-held check below is really a polarity-consistency check.
//
//   reverse = false (default): pressed = pin LOW
//   reverse = true:            pressed = pin HIGH
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

OpenSkyhawk::ActionButton gBtn(CTRL_ID, PinRef(PIN_BTN), /*reverse=*/true);

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== ActionButton reverse ===");
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

    // reverse = true → LOW is the RELEASED state
    digitalWrite(PIN_CTRL, LOW);
    delayMicroseconds(100);
    gBtn.forceReport();
    flushDrain();
    check("reverse: forceReport() at LOW emits nothing", gEvtCount == 0);

    // LOW must NOT count as a press under reverse
    delay(OpenSkyhawk::ActionButton::DEBOUNCE_MS * 2);
    gBtn.poll();
    flushDrain();
    check("reverse: LOW does not fire", gEvtCount == 0);

    // HIGH is the press
    digitalWrite(PIN_CTRL, HIGH);
    delayMicroseconds(100);
    gBtn.poll();
    flushDrain();
    check("reverse: HIGH fires", gEvtCount == 1);

    // Held HIGH stays at one
    delay(OpenSkyhawk::ActionButton::DEBOUNCE_MS * 3);
    for (int i = 0; i < 20; i++) { gBtn.poll(); delay(1); }
    flushDrain();
    check("reverse: held HIGH still 1 EVT", gEvtCount == 1);

    // Return to LOW = release, silent
    digitalWrite(PIN_CTRL, LOW);
    delayMicroseconds(100);
    gBtn.poll();
    flushDrain();
    check("reverse: release at LOW is silent", gEvtCount == 1);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
