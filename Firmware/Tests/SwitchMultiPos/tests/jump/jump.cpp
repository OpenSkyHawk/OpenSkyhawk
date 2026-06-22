// SwitchMultiPos — jump test
//
// The emitted value is the ABSOLUTE index, not a delta. A non-adjacent jump (pos 1 -> pos 3,
// with position 2 never read) emits exactly 3 — no adjacency assumption, nothing lost on a skip.
// Verified via position()/emitCount(); CAN in NORMAL mode (node ACKs the PanelBridge).
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

static void settle(uint8_t idx) { setActive(idx); gSel.poll(); delay(25); gSel.poll(); txPush(); }

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== SwitchMultiPos jump ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    for (uint8_t i = 0; i < N; i++) pinMode(CTRL_PINS[i], OUTPUT);
    setActive(0);
    gSel.configure();
    CANProtocol::start();

    gSel.forceReport();        // baseline at pos 0
    settle(1);                 // now confirmed at pos 1
    check("settled at pos 1", gSel.position() == 1);

    uint16_t before = gSel.emitCount();
    settle(3);                 // jump 1 -> 3 directly; position 2 is never driven/polled
    check("jump 1->3 emits index 3", gSel.position() == 3);
    check("jump 1->3 is a single EVT (no intermediate 2)", gSel.emitCount() == before + 1);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
