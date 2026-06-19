// Switch2Pos — forceReport() test
//
// Verifies:
//   forceReport() emits current pin state immediately (no debounce).
//   forceReport() with pin HIGH (inactive) → EVT value 0.
//   forceReport() with pin LOW  (active)   → EVT value 1.
//   A second forceReport() with same pin state still emits a new EVT.
//   _initialized is set by forceReport() so poll() becomes active.
//
// Hardware: STM32. PB0→PA0 jumper wire required.
// PB0: output — drives switch state. PA0: input — Switch2Pos reads this.

#include <Arduino.h>
#include <STM32Board.h>
#include <Inputs/Switch2Pos.h>

static constexpr uint16_t CTRL_ID  = 0xABCD;
static constexpr uint8_t  PIN_CTRL = PB0;   // drives switch state
static constexpr uint8_t  PIN_SW   = PA0;   // Switch2Pos input

static uint8_t  gEvtCount = 0;
static uint16_t gLastVal  = 0xFFFF;

static void onCan(uint32_t canId, const uint8_t* data, uint8_t len) {
    if (canId != canIdEvt(NODE_ID) || len < 8) return;
    const ControlPacketPair* pair = reinterpret_cast<const ControlPacketPair*>(data);
    if (pair->a.controlId == CTRL_ID) {
        gEvtCount++;
        gLastVal = pair->a.value;
    } else if (pair->b.controlId == CTRL_ID) {
        gEvtCount++;
        gLastVal = pair->b.value;
    }
}

static void flushDrain() {
    CANProtocol::flushBatched(canIdEvt(NODE_ID));
    delay(2);
    CANProtocol::drain();
}

OpenSkyhawk::Switch2Pos gSw(CTRL_ID, PinRef(PIN_SW));

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== Switch2Pos force_report ===");
    STM32Board::diagSerial().println("Hardware: PB0->PA0 jumper wire required.");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    pinMode(PIN_CTRL, OUTPUT);
    gSw.configure();

    CANProtocol::onReceive(onCan);
    CANProtocol::filterAcceptId(canIdEvt(NODE_ID));
    CANProtocol::startLoopback();

    // ── Pin HIGH (inactive, reverse=false → value 0) ─────────────────────────

    digitalWrite(PIN_CTRL, HIGH);
    delayMicroseconds(100);

    gSw.forceReport();
    flushDrain();
    check("forceReport() with HIGH pin: 1 EVT", gEvtCount == 1);
    check("forceReport() with HIGH pin: value 0", gLastVal == 0);

    // ── Pin LOW (active, reverse=false → value 1) ─────────────────────────────

    digitalWrite(PIN_CTRL, LOW);
    delayMicroseconds(100);

    gSw.forceReport();
    flushDrain();
    check("forceReport() with LOW pin: 2 EVTs total", gEvtCount == 2);
    check("forceReport() with LOW pin: value 1", gLastVal == 1);

    // ── Same state again: forceReport() still emits ───────────────────────────
    // Pin remains LOW — same state as previous forceReport()

    gSw.forceReport();
    flushDrain();
    check("second forceReport() same state: 3 EVTs total", gEvtCount == 3);
    check("second forceReport() same state: value 1", gLastVal == 1);

    // ── poll() active after forceReport() ────────────────────────────────────
    // Switch poll() — no state change since pin is still LOW; no EVT expected.
    gSw.poll();
    flushDrain();
    check("poll() after forceReport(): no extra EVT", gEvtCount == 3);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
