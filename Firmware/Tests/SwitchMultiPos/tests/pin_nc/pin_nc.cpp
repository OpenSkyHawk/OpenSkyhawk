// SwitchMultiPos — pin_nc test
//
// A PIN_NC entry marks a mechanical-only detent (a position with no physical pin). When no
// electrical pin is active, that detent's index is reported; electrical positions override it.
// Verified via position(); CAN in NORMAL mode (node ACKs the PanelBridge).
//
// pins = { PA0 (pos 0), PIN_NC (pos 1), PA4 (pos 2) }.
// Rig: STM32 on the CAN bus with the PanelBridge. Jumper PB0->PA0, PB4->PA4 (pos 1 has no pin).

#include <Arduino.h>
#include <STM32Board.h>
#include <Inputs/SwitchMultiPos/SwitchMultiPos.h>

static constexpr uint16_t CTRL_ID = 0x5678;
static constexpr uint8_t  N       = 3;
static constexpr uint8_t  NONE    = 0xFF;

static void setActive(uint8_t idx) {   // idx 0 → PA0 active, idx 2 → PA4 active; 1/NONE → none
    digitalWrite(PB0,  idx == 0 ? LOW : HIGH);
    digitalWrite(PB4, idx == 2 ? LOW : HIGH);
    delayMicroseconds(100);
}
static void txPush() { CANProtocol::flushBatched(canIdEvt(NODE_ID)); }

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

    pinMode(PB0,  OUTPUT);
    pinMode(PB4, OUTPUT);
    gSel.configure();
    CANProtocol::start();

    setActive(NONE);
    gSel.forceReport(); txPush();
    check("no electrical pin: reports NC detent index 1", gSel.position() == 1);

    setActive(0);
    gSel.forceReport(); txPush();
    check("PA0 active: index 0", gSel.position() == 0);

    setActive(2);
    gSel.forceReport(); txPush();
    check("PA4 active: index 2", gSel.position() == 2);

    setActive(NONE);
    gSel.forceReport(); txPush();
    check("back to NC detent: index 1", gSel.position() == 1);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
