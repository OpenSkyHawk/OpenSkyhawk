// SwitchMultiPos — forceReport test
//
// forceReport() resolves the current position and emits immediately (no debounce); it emits
// even when the position is unchanged, and arms poll().
//
// Hardware: STM32. Jumper PB0->PA0, PB1->PA1, PB4->PA4, PB5->PA5.

#include <Arduino.h>
#include <STM32Board.h>
#include <Inputs/SwitchMultiPos/SwitchMultiPos.h>

static constexpr uint16_t CTRL_ID = 0x5678;
static constexpr uint8_t  N       = 4;
static const uint8_t SW_PINS[N]   = { PA0, PA1, PA4, PA5 };
static const uint8_t CTRL_PINS[N] = { PB0, PB1, PB4, PB5 };

static uint8_t  gEvtCount = 0;
static uint16_t gLastVal  = 0xFFFF;

static void onCan(uint32_t canId, const uint8_t* data, uint8_t len) {
    if (canId != canIdEvt(NODE_ID) || len < 8) return;
    const ControlPacketPair* pair = reinterpret_cast<const ControlPacketPair*>(data);
    if (pair->a.controlId == CTRL_ID)      { gEvtCount++; gLastVal = pair->a.value; }
    else if (pair->b.controlId == CTRL_ID) { gEvtCount++; gLastVal = pair->b.value; }
}

static void flushDrain() { CANProtocol::flushBatched(canIdEvt(NODE_ID)); delay(2); CANProtocol::drain(); }

static void setActive(uint8_t idx) {
    for (uint8_t i = 0; i < N; i++) digitalWrite(CTRL_PINS[i], i == idx ? LOW : HIGH);
    delayMicroseconds(100);
}

static const PinRef gPins[N] = { PinRef(SW_PINS[0]), PinRef(SW_PINS[1]), PinRef(SW_PINS[2]), PinRef(SW_PINS[3]) };
OpenSkyhawk::SwitchMultiPos gSel(CTRL_ID, gPins, N);

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== SwitchMultiPos force_report ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    for (uint8_t i = 0; i < N; i++) pinMode(CTRL_PINS[i], OUTPUT);
    gSel.configure();

    CANProtocol::onReceive(onCan);
    CANProtocol::filterAcceptId(canIdEvt(NODE_ID));
    CANProtocol::startLoopback();

    setActive(2);
    gSel.forceReport();
    flushDrain();
    check("forceReport at pos 2: 1 EVT", gEvtCount == 1);
    check("forceReport at pos 2: value 2", gLastVal == 2);

    // Same position again — forceReport still emits.
    gSel.forceReport();
    flushDrain();
    check("second forceReport same pos: 2 EVTs total", gEvtCount == 2);
    check("second forceReport same pos: value 2", gLastVal == 2);

    // poll() armed, no change → no extra EVT.
    gSel.poll();
    flushDrain();
    check("poll() after forceReport, no change: no extra EVT", gEvtCount == 2);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
