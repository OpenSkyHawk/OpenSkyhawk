// SwitchMultiPos — reverse test
//
// reverse = true inverts the active level: the active pin reads HIGH (not LOW). The resolved
// index is unchanged; only the electrical sense flips. Verified via position(); CAN in NORMAL
// mode (node ACKs the PanelBridge).
//
// Rig: STM32 on the CAN bus with the PanelBridge. Jumper PB0->PA0, PB1->PA1, PB10->PA4, PB5->PA5.

#include <Arduino.h>
#include <STM32Board.h>
#include <Inputs/SwitchMultiPos/SwitchMultiPos.h>

static constexpr uint16_t CTRL_ID = 0x5678;
static constexpr uint8_t  N       = 4;
static const uint8_t SW_PINS[N]   = { PA0, PA1, PA4, PA5 };
static const uint8_t CTRL_PINS[N] = { PB0, PB1, PB10, PB5 };

// reverse=true → active pin is HIGH. Drive position idx HIGH (active), the rest LOW.
static void setActiveHigh(uint8_t idx) {
    for (uint8_t i = 0; i < N; i++) digitalWrite(CTRL_PINS[i], i == idx ? HIGH : LOW);
    delayMicroseconds(100);
}
static void txPush() { CANProtocol::flushBatched(canIdEvt(NODE_ID)); }

static const PinRef gPins[N] = { PinRef(SW_PINS[0]), PinRef(SW_PINS[1]), PinRef(SW_PINS[2]), PinRef(SW_PINS[3]) };
OpenSkyhawk::SwitchMultiPos gSel(CTRL_ID, gPins, N, /*reverse=*/true);

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== SwitchMultiPos reverse ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    for (uint8_t i = 0; i < N; i++) pinMode(CTRL_PINS[i], OUTPUT);
    gSel.configure();
    CANProtocol::start();

    for (uint8_t i = 0; i < N; i++) {
        setActiveHigh(i);
        gSel.forceReport(); txPush();
        char lbl[44];
        snprintf(lbl, sizeof(lbl), "reverse: pin %u HIGH -> index %u", i, gSel.position());
        check(lbl, gSel.position() == i);
    }

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
