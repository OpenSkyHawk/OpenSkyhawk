// SwitchMultiPos — forceReport test
//
// forceReport() resolves the current position and emits immediately; it re-emits even when the
// position is unchanged, and arms poll(). Verified via position()/emitCount().
//
// Rig: STM32 on the CAN bus with the PanelBridge. Jumper PB0->PA0, PB1->PA1, PB10->PA4, PB5->PA5.

#include <Arduino.h>
#include <STM32Board.h>
#include <Inputs/SwitchMultiPos/SwitchMultiPos.h>

static constexpr uint16_t CTRL_ID = 0x5678;
static constexpr uint8_t  N       = 4;
static const uint8_t SW_PINS[N]   = { PA0, PA1, PA4, PA5 };
static const uint8_t CTRL_PINS[N] = { PB0, PB1, PB10, PB5 };

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
    STM32Board::diagSerial().println("=== SwitchMultiPos force_report ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    for (uint8_t i = 0; i < N; i++) pinMode(CTRL_PINS[i], OUTPUT);
    gSel.configure();
    CANProtocol::start();

    setActive(2);
    gSel.forceReport(); txPush();
    check("forceReport pos 2: 1 EVT, index 2", gSel.emitCount() == 1 && gSel.position() == 2);

    // Same position again — forceReport still emits.
    gSel.forceReport(); txPush();
    check("second forceReport same pos: 2 EVTs total", gSel.emitCount() == 2 && gSel.position() == 2);

    // poll() armed, no change → no extra EVT.
    gSel.poll();
    check("poll() after forceReport, no change: no extra EVT", gSel.emitCount() == 2);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
