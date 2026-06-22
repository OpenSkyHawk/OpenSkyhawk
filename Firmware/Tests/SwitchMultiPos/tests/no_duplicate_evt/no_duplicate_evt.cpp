// SwitchMultiPos — no_duplicate_evt test
//
// A held position emits exactly one EVT (emit-on-change). Repeated poll() while the selector
// is stationary produces no further traffic.
//
// Hardware: STM32. Jumper PB0->PA0, PB1->PA1, PB10->PA4, PB5->PA5.

#include <Arduino.h>
#include <STM32Board.h>
#include <Inputs/SwitchMultiPos/SwitchMultiPos.h>

static constexpr uint16_t CTRL_ID = 0x5678;
static constexpr uint8_t  N       = 4;
static const uint8_t SW_PINS[N]   = { PA0, PA1, PA4, PA5 };
static const uint8_t CTRL_PINS[N] = { PB0, PB1, PB10, PB5 };

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
    STM32Board::diagSerial().println("=== SwitchMultiPos no_duplicate_evt ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    for (uint8_t i = 0; i < N; i++) pinMode(CTRL_PINS[i], OUTPUT);
    setActive(0);
    gSel.configure();

    CANProtocol::onReceive(onCan);
    CANProtocol::filterAcceptId(canIdEvt(NODE_ID));
    CANProtocol::startLoopback();

    gSel.forceReport();            // baseline at pos 0

    // Move to pos 3 and confirm — one EVT.
    setActive(3);
    gSel.poll();
    delay(25);
    gSel.poll();
    flushDrain();
    uint8_t afterChange = gEvtCount;
    check("change to pos 3: EVT value 3", gLastVal == 3);

    // Hold pos 3 and poll repeatedly — no further EVTs.
    for (uint8_t i = 0; i < 8; i++) { gSel.poll(); delay(5); }
    flushDrain();
    check("held pos 3: no duplicate EVTs", gEvtCount == afterChange);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
