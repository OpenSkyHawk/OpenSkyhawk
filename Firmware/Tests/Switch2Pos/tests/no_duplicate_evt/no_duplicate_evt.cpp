// Switch2Pos — no duplicate EVT test
//
// Verifies:
//   Once an EVT is emitted, subsequent poll() calls with the same stable state
//   do not re-emit. EVT fires exactly once per confirmed state change.
//   After a second confirmed state change, exactly one more EVT is emitted.
//
// Hardware: STM32. PB0→PA0 jumper wire required.

#include <Arduino.h>
#include <STM32Board.h>
#include <Inputs/Switch2Pos/Switch2Pos.h>

static constexpr uint16_t CTRL_ID  = 0xABCD;
static constexpr uint8_t  PIN_CTRL = PB0;
static constexpr uint8_t  PIN_SW   = PA0;

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
    STM32Board::diagSerial().println("=== Switch2Pos no_duplicate_evt ===");
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

    // ── Init: pin HIGH (inactive, value=0) ───────────────────────────────────

    digitalWrite(PIN_CTRL, HIGH);
    delayMicroseconds(100);
    gSw.forceReport();
    flushDrain();
    check("forceReport() HIGH: 1 EVT (value 0)", gEvtCount == 1 && gLastVal == 0);

    // ── Multiple poll() in stable state: no extra EVTs ───────────────────────

    for (int i = 0; i < 20; i++) gSw.poll();
    flushDrain();
    check("20x poll() in stable state: still 1 EVT total", gEvtCount == 1);

    // ── State change → exactly one EVT ───────────────────────────────────────

    digitalWrite(PIN_CTRL, LOW);
    delayMicroseconds(100);
    gSw.poll();    // debounce starts
    delay(25);
    gSw.poll();    // confirms, emits
    flushDrain();
    check("confirmed state change: exactly 2 EVTs total", gEvtCount == 2);
    check("confirmed state change: value 1", gLastVal == 1);

    // ── Multiple poll() after second state confirmed: no extra EVTs ───────────

    for (int i = 0; i < 20; i++) gSw.poll();
    flushDrain();
    check("20x poll() after second confirm: still 2 EVTs total", gEvtCount == 2);

    // ── Second state change → exactly one more EVT ───────────────────────────

    digitalWrite(PIN_CTRL, HIGH);
    delayMicroseconds(100);
    gSw.poll();    // debounce starts
    delay(25);
    gSw.poll();    // confirms, emits
    flushDrain();
    check("third confirmed change: exactly 3 EVTs total", gEvtCount == 3);
    check("third confirmed change: value 0", gLastVal == 0);

    for (int i = 0; i < 20; i++) gSw.poll();
    flushDrain();
    check("20x poll() after third confirm: still 3 EVTs total", gEvtCount == 3);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
