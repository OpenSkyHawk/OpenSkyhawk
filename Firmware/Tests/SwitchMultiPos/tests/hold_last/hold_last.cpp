// SwitchMultiPos — hold_last test
//
// When no pin reads active (a non-shorting rotary mid-throw), the last confirmed position is
// held: readRaw() returns NO_POSITION and the base keeps position() — no EVT. Verified via
// position()/emitCount(); CAN in NORMAL mode (node ACKs the PanelBridge).
//
// Rig: STM32 on the CAN bus with the PanelBridge. Jumper PB0->PA0, PB1->PA1, PB4->PA4, PB5->PA5.

#include <Arduino.h>
#include <STM32Board.h>
#include <Inputs/SwitchMultiPos/SwitchMultiPos.h>

static constexpr uint16_t CTRL_ID = 0x5678;
static constexpr uint8_t  N       = 4;
static constexpr uint8_t  NONE    = 0xFF;   // setActive(NONE) → all pins HIGH (inactive)
static const uint8_t SW_PINS[N]   = { PA0, PA1, PA4, PA5 };
static const uint8_t CTRL_PINS[N] = { PB0, PB1, PB4, PB5 };

static void setActive(uint8_t idx) {
    for (uint8_t i = 0; i < N; i++) digitalWrite(CTRL_PINS[i], i == idx ? LOW : HIGH);
    delayMicroseconds(100);
}
static void txPush() { CANProtocol::flushBatched(canIdEvt(NODE_ID)); }

static const PinRef gPins[N] = { PinRef(SW_PINS[0]), PinRef(SW_PINS[1]), PinRef(SW_PINS[2]), PinRef(SW_PINS[3]) };
OpenSkyhawk::SwitchMultiPos gSel(CTRL_ID, gPins, N);

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== SwitchMultiPos hold_last ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    for (uint8_t i = 0; i < N; i++) pinMode(CTRL_PINS[i], OUTPUT);
    gSel.configure();
    CANProtocol::start();

    setActive(1);
    gSel.forceReport(); txPush();
    check("baseline at pos 1", gSel.emitCount() == 1 && gSel.position() == 1);

    // Open all contacts (mid-throw gap) — hold last, no EVT.
    setActive(NONE);
    gSel.poll(); delay(25); gSel.poll();
    check("all pins open: no EVT", gSel.emitCount() == 1);
    check("all pins open: still reports pos 1", gSel.position() == 1);

    // Land on position 3 → emit 3.
    setActive(3);
    gSel.poll(); delay(25); gSel.poll(); txPush();
    check("after gap, land pos 3", gSel.emitCount() == 2 && gSel.position() == 3);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
