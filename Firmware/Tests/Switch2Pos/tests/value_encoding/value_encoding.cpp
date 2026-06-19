// Switch2Pos — value_encoding test
//
// Verifies (reverse = false, default):
//   Pin LOW  (switch closed, active)   → EVT value 1
//   Pin HIGH (switch open,  inactive)  → EVT value 0
//   controlId is forwarded correctly to CANProtocol::sendBatched().
//
// Hardware: STM32. PB0→PA0 jumper wire required.

#include <Arduino.h>
#include <STM32Board.h>
#include <Inputs/Switch2Pos.h>

static constexpr uint16_t CTRL_ID  = 0x1234;
static constexpr uint8_t  PIN_CTRL = PB0;
static constexpr uint8_t  PIN_SW   = PA0;

static uint8_t  gEvtCount    = 0;
static uint16_t gLastVal     = 0xFFFF;
static uint16_t gLastCtrlId  = 0xFFFF;

static void onCan(uint32_t canId, const uint8_t* data, uint8_t len) {
    if (canId != canIdEvt(NODE_ID) || len < 8) return;
    const ControlPacketPair* pair = reinterpret_cast<const ControlPacketPair*>(data);
    if (pair->a.controlId == CTRL_ID) {
        gEvtCount++;
        gLastVal    = pair->a.value;
        gLastCtrlId = pair->a.controlId;
    } else if (pair->b.controlId == CTRL_ID) {
        gEvtCount++;
        gLastVal    = pair->b.value;
        gLastCtrlId = pair->b.controlId;
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
    STM32Board::diagSerial().println("=== Switch2Pos value_encoding ===");
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

    // ── Pin HIGH (inactive) → value 0 ────────────────────────────────────────

    digitalWrite(PIN_CTRL, HIGH);
    delayMicroseconds(100);
    gSw.forceReport();
    flushDrain();
    check("HIGH pin: EVT value == 0", gLastVal == 0);
    check("controlId forwarded correctly (HIGH)", gLastCtrlId == CTRL_ID);

    // ── Pin LOW (active) → value 1 ────────────────────────────────────────────

    digitalWrite(PIN_CTRL, LOW);
    delayMicroseconds(100);
    gSw.forceReport();
    flushDrain();
    check("LOW pin: EVT value == 1", gLastVal == 1);
    check("controlId forwarded correctly (LOW)", gLastCtrlId == CTRL_ID);

    // ── Confirm via debounced poll() ──────────────────────────────────────────
    // Transition from LOW back to HIGH, let debounce expire.

    digitalWrite(PIN_CTRL, HIGH);
    delayMicroseconds(100);
    gSw.poll();    // start debounce
    delay(25);     // let window expire
    uint8_t countBefore = gEvtCount;
    gSw.poll();    // should confirm and emit
    flushDrain();
    check("poll() after debounce: value 0", gEvtCount == countBefore + 1 && gLastVal == 0);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
