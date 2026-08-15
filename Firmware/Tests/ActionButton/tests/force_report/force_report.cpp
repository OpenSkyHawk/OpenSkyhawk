// ActionButton — forceReport() silence and boot-held safety
//
// Verifies both halves of the forceReport() contract, which differs from every other input class:
//   forceReport() emits NOTHING — released or pressed, first call or tenth.
//     Every other class emits current position here, which is idempotent for a switch. An action
//     is not: emitting would flip the sim switch on every SYNC_REQ.
//   forceReport() still SEEDS the baseline — it is not a no-op.
//     A button physically held at boot must not read as a press edge on the first poll(). Without
//     the seed, _lastPressed would be false against a pressed pin and the node would fire an
//     unwanted TOGGLE the moment it started polling.
//   poll() is inert until forceReport() has run (the _initialized lifecycle).
//
// Hardware: STM32. PB0→PA0 jumper wire required.
// PB0: output — drives button state. PA0: input — ActionButton reads this.

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
    STM32Board::diagSerial().println("=== ActionButton force_report ===");
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

    // ── poll() inert before forceReport() ────────────────────────────────────
    digitalWrite(PIN_CTRL, LOW);          // pressed
    delayMicroseconds(100);
    gBtn.poll();
    flushDrain();
    check("poll() before forceReport(): no EVT", gEvtCount == 0);

    // ── BOOT-HELD: button already pressed when forceReport() runs ────────────
    // Seeds _lastPressed = pressed. The first poll() must then see no edge.
    gBtn.forceReport();
    flushDrain();
    check("forceReport() while pressed: no EVT", gEvtCount == 0);

    delay(OpenSkyhawk::ActionButton::DEBOUNCE_MS * 2);
    gBtn.poll();
    flushDrain();
    check("boot-held: first poll() does not fire", gEvtCount == 0);

    // Releasing a boot-held button must also stay silent — release is never an event.
    digitalWrite(PIN_CTRL, HIGH);
    delayMicroseconds(100);
    gBtn.poll();
    flushDrain();
    check("boot-held: release does not fire", gEvtCount == 0);

    // ...and the button is now armed: a genuine press fires.
    delay(OpenSkyhawk::ActionButton::DEBOUNCE_MS * 2);
    digitalWrite(PIN_CTRL, LOW);
    delayMicroseconds(100);
    gBtn.poll();
    flushDrain();
    check("boot-held: next real press fires", gEvtCount == 1);

    // ── Repeated forceReport() stays silent (SYNC_REQ arrives repeatedly) ────
    const uint8_t before = gEvtCount;
    for (int i = 0; i < 5; i++) { gBtn.forceReport(); flushDrain(); }
    check("5x forceReport(): no further EVT", gEvtCount == before);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
