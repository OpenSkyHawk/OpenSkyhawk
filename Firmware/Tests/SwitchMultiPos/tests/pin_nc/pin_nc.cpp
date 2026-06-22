// SwitchMultiPos — pin_nc test
//
// A PIN_NC entry marks a mechanical-only detent (a position with no physical pin). When no
// electrical pin is active, that detent's index is reported; electrical positions override it.
//
// pins = { PA0 (pos 0), PIN_NC (pos 1), PA4 (pos 2) }.
// Hardware: STM32. Jumper PB0->PA0, PB10->PA4. (Position 1 has no switch pin.)

#include <Arduino.h>
#include <STM32Board.h>
#include <Inputs/SwitchMultiPos/SwitchMultiPos.h>

static constexpr uint16_t CTRL_ID = 0x5678;
static constexpr uint8_t  N       = 3;
static constexpr uint8_t  NONE    = 0xFF;

static uint8_t  gEvtCount = 0;
static uint16_t gLastVal  = 0xFFFF;

static void onCan(uint32_t canId, const uint8_t* data, uint8_t len) {
    if (canId != canIdEvt(NODE_ID) || len < 8) return;
    const ControlPacketPair* pair = reinterpret_cast<const ControlPacketPair*>(data);
    if (pair->a.controlId == CTRL_ID)      { gEvtCount++; gLastVal = pair->a.value; }
    else if (pair->b.controlId == CTRL_ID) { gEvtCount++; gLastVal = pair->b.value; }
}

static void flushDrain() { CANProtocol::flushBatched(canIdEvt(NODE_ID)); delay(2); CANProtocol::drain(); }

// idx 0 drives PA0 active, idx 2 drives PA4 active; idx 1/NONE → nothing electrical active.
static void setActive(uint8_t idx) {
    digitalWrite(PB0, idx == 0 ? LOW : HIGH);
    digitalWrite(PB10, idx == 2 ? LOW : HIGH);
    delayMicroseconds(100);
}

static const PinRef gPins[N] = { PinRef(PA0), PIN_NC, PinRef(PA4) };
OpenSkyhawk::SwitchMultiPos gSel(CTRL_ID, gPins, N);

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== SwitchMultiPos pin_nc ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    pinMode(PB0, OUTPUT);
    pinMode(PB10, OUTPUT);
    gSel.configure();

    CANProtocol::onReceive(onCan);
    CANProtocol::filterAcceptId(canIdEvt(NODE_ID));
    CANProtocol::startLoopback();

    // Nothing electrical active → the PIN_NC detent (index 1) is reported.
    setActive(NONE);
    gSel.forceReport();
    flushDrain();
    check("no electrical pin: reports NC detent index 1", gLastVal == 1);

    // Electrical positions still resolve.
    setActive(0);
    gSel.forceReport();
    flushDrain();
    check("PA0 active: index 0", gLastVal == 0);

    setActive(2);
    gSel.forceReport();
    flushDrain();
    check("PA4 active: index 2", gLastVal == 2);

    // Back to no electrical pin → NC detent again.
    setActive(NONE);
    gSel.forceReport();
    flushDrain();
    check("back to NC detent: index 1", gLastVal == 1);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
