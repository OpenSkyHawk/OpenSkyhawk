// AnalogMultiPos — force_report test
//
// debugSetRaw() injects an ADC reading; forceReport() resolves it and emits immediately. Normal
// CAN (node ACKs the PanelBridge). Verified via position()/emitCount(). No analog hardware needed.

#include <Arduino.h>
#include <STM32Board.h>
#include <Inputs/AnalogMultiPos/AnalogMultiPos.h>

using namespace OpenSkyhawk;

static constexpr uint16_t CTRL_ID = 0x803b;
static const uint16_t POSVALS[] = { 4000, 16000, 42000, 56000 };
AnalogMultiPos gSel(CTRL_ID, PinRef(PA1), 4, POSVALS);

static void txPush() { CANProtocol::flushBatched(canIdEvt(NODE_ID)); }

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== AnalogMultiPos force_report ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    gSel.configure();
    CANProtocol::start();

    gSel.debugSetRaw(16000);
    gSel.forceReport(); txPush();
    check("forceReport at 16000 -> index 1, 1 EVT", gSel.emitCount() == 1 && gSel.position() == 1);

    gSel.debugSetRaw(42000);
    gSel.forceReport(); txPush();
    check("forceReport at 42000 -> index 2, 2 EVTs", gSel.emitCount() == 2 && gSel.position() == 2);

    gSel.poll();
    check("poll() no change: no extra EVT", gSel.emitCount() == 2);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
