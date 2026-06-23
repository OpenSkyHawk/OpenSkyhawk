// AnalogMultiPos — change_emit test
//
// Full poll path on the MultiPosInput base: emit only on a resolved-position change; a held
// reading emits no duplicate; a reading in the deadband gap holds the last position (hysteresis).
// debugSetRaw injects the ADC value; normal CAN. Verified via position()/emitCount().

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
    STM32Board::diagSerial().println("=== AnalogMultiPos change_emit ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    gSel.configure();
    CANProtocol::start();

    gSel.debugSetRaw(4000);
    gSel.forceReport();                  // baseline at pos 0
    uint16_t base = gSel.emitCount();

    // Change to pos 2 (42000). debounceMs=0 → confirm on the next poll.
    gSel.debugSetRaw(42000);
    gSel.poll(); gSel.poll(); txPush();
    check("change to 42000 -> index 2, one EVT", gSel.emitCount() == base + 1 && gSel.position() == 2);

    // Held reading — repeated poll, no duplicate EVT.
    for (uint8_t i = 0; i < 5; i++) gSel.poll();
    check("held: no duplicate EVT", gSel.emitCount() == base + 1);

    // Reading in the deadband gap (10000, between pos 0 and 1) → holds last (2), no EVT.
    gSel.debugSetRaw(10000);
    gSel.poll(); gSel.poll();
    check("gap reading: holds pos 2, no EVT", gSel.emitCount() == base + 1 && gSel.position() == 2);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
