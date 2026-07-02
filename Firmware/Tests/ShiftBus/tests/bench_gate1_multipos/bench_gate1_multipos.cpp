// ShiftBus — bench gate 1: simulated multipos selector through a real 74HC165
//
// SwitchMultiPos on '165 chip 0, D0–D4, one-hot: each input has a 10 k pull-up to 3V3;
// "select" a position by jumpering that D-pin to GND (no real rotary switch wasted).
// Full production path: PanelGroup::setup()/loop() → cached SR reads → debounce →
// one-hot decode → CAN EVT to the PanelBridge.
//
// Rig: 74HC165 on the standard pins (SCK=PB3 MISO=PB4 MOSI=PB5 LOAD=PB8), CAN bus to the
// PanelBridge. Watch the diag serial: position prints on every confirmed change.
//
// PASS (gate 1): reported position matches the grounded pin, no phantom positions while
// moving the jumper, boot forceReport shows the initial position.

#include <Arduino.h>
#include <STM32Board.h>
#include <PanelGroup.h>
#include <Inputs/SwitchMultiPos/SwitchMultiPos.h>

static constexpr uint16_t CTRL_ID = 0x3A01;   // bench id — DOPPLER_SEL stand-in

const PinRef SEL_PINS[] = {
    PinRef(ShiftBus1, 0, 0),
    PinRef(ShiftBus1, 0, 1),
    PinRef(ShiftBus1, 0, 2),
    PinRef(ShiftBus1, 0, 3),
    PinRef(ShiftBus1, 0, 4),
};

OpenSkyhawk::SwitchMultiPos gSel(CTRL_ID, SEL_PINS, 5);

void setup() {
    STM32Board::setDebug(true);
    PanelGroup::setup();   // zero-setup lifecycle: the SR PinRefs announced ShiftBus1
    STM32Board::diagSerial().println("=== ShiftBus bench gate 1: '165 multipos (jumper D0-D4 to GND) ===");
}

void loop() {
    PanelGroup::loop();

    // 1 Hz raw-frame peek so a wiring fault is visible even before a valid one-hot state.
    static uint32_t lastPrint = 0;
    if (millis() - lastPrint >= 1000) {
        lastPrint = millis();
        auto& d = STM32Board::diagSerial();
        d.print(F("[gate1] D4..D0 = "));
        for (int8_t b = 4; b >= 0; b--) d.print(SEL_PINS[b].read() ? '1' : '0');
        d.println();
    }
}
