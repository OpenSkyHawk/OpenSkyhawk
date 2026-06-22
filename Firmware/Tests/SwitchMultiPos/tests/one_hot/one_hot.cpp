// SwitchMultiPos — one_hot test
//
// Drives each of N position pins active (LOW, reverse=false) in turn and confirms the
// emitted EVT carries that position's index. forceReport() is used so each read emits
// immediately (the debounced poll() path is covered by the debounce/jump/fast_sweep tests).
//
// Hardware: STM32. Jumper PB0->PA0, PB1->PA1, PB10->PA4, PB5->PA5.
//   PBx: outputs driving the switch lines.  PAx: SwitchMultiPos inputs (one-hot).

#include <Arduino.h>
#include <STM32Board.h>
#include <Inputs/SwitchMultiPos/SwitchMultiPos.h>

static constexpr uint16_t CTRL_ID = 0x5678;
static constexpr uint8_t  N       = 4;
static const uint8_t SW_PINS[N]   = { PA0, PA1, PA4, PA5 };
static const uint8_t CTRL_PINS[N] = { PB0, PB1, PB10, PB5 };

static uint8_t  gEvtCount   = 0;
static uint16_t gLastVal    = 0xFFFF;
static uint16_t gLastCtrlId = 0xFFFF;

static void onCan(uint32_t canId, const uint8_t* data, uint8_t len) {
    if (canId != canIdEvt(NODE_ID) || len < 8) return;
    const ControlPacketPair* pair = reinterpret_cast<const ControlPacketPair*>(data);
    if (pair->a.controlId == CTRL_ID)      { gEvtCount++; gLastVal = pair->a.value; gLastCtrlId = pair->a.controlId; }
    else if (pair->b.controlId == CTRL_ID) { gEvtCount++; gLastVal = pair->b.value; gLastCtrlId = pair->b.controlId; }
}

static void flushDrain() { CANProtocol::flushBatched(canIdEvt(NODE_ID)); delay(2); CANProtocol::drain(); }

// Drive position idx LOW (active), the rest HIGH.
static void setActive(uint8_t idx) {
    for (uint8_t i = 0; i < N; i++) digitalWrite(CTRL_PINS[i], i == idx ? LOW : HIGH);
    delayMicroseconds(100);
}

static const PinRef gPins[N] = { PinRef(SW_PINS[0]), PinRef(SW_PINS[1]), PinRef(SW_PINS[2]), PinRef(SW_PINS[3]) };
OpenSkyhawk::SwitchMultiPos gSel(CTRL_ID, gPins, N);

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== SwitchMultiPos one_hot ===");
    STM32Board::diagSerial().println("Hardware: jumper PB0->PA0, PB1->PA1, PB10->PA4, PB5->PA5.");

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

    for (uint8_t i = 0; i < N; i++) {
        setActive(i);
        gSel.forceReport();
        flushDrain();
        char lbl[40];
        snprintf(lbl, sizeof(lbl), "pos %u active -> index %u", i, gLastVal);
        check(lbl, gLastVal == i);
    }
    check("controlId forwarded", gLastCtrlId == CTRL_ID);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
