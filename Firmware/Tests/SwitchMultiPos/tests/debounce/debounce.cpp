// SwitchMultiPos — debounce test
//
// A changed position must hold steady for DEBOUNCE_MS (20 ms) before it is confirmed and
// emitted; a transient that returns to the current position before the window expires emits
// nothing. Verified via position()/emitCount(); CAN in NORMAL mode (node ACKs the PanelBridge).
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
    STM32Board::diagSerial().println("=== SwitchMultiPos debounce ===");

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

    gSel.forceReport(); txPush();        // baseline at pos 0
    uint16_t base = gSel.emitCount();    // == 1

    // Change to pos 2 — not emitted until the 20 ms window expires.
    setActive(2);
    gSel.poll();                         // arms debounce
    check("poll() right after change: no EVT yet", gSel.emitCount() == base && gSel.position() == 0);

    delay(25);
    gSel.poll(); txPush();               // window expired → confirm + emit
    check("after 20 ms: EVT index 2", gSel.emitCount() == base + 1 && gSel.position() == 2);

    // Bounce: from settled 2, blip to 0 then back to 2 within the window → no EVT.
    uint16_t before = gSel.emitCount();
    setActive(0); gSel.poll();           // pending = 0, timer restarts
    setActive(2); gSel.poll();           // pending = 2 again (== confirmed); never left long enough
    delay(25);
    gSel.poll();
    check("blip 2->0->2 within window: no EVT", gSel.emitCount() == before && gSel.position() == 2);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
