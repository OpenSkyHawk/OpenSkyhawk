// ShiftBus — bench probe: live dump of a 2-chip '165 cascade.
// Prints both input bytes on any change (D7..D0 per chip) + a 2 s heartbeat.
// Poke a switch / jumper an input to 3V3 and watch its bit — chip 1 bits prove the
// QH->SER cascade hop.

#include <Arduino.h>
#include <STM32Board.h>
#include <PanelGroup.h>

PinRef gIn[16] = {
    PinRef(ShiftBus1, 0, 0), PinRef(ShiftBus1, 0, 1), PinRef(ShiftBus1, 0, 2),
    PinRef(ShiftBus1, 0, 3), PinRef(ShiftBus1, 0, 4), PinRef(ShiftBus1, 0, 5),
    PinRef(ShiftBus1, 0, 6), PinRef(ShiftBus1, 0, 7),
    PinRef(ShiftBus1, 1, 0), PinRef(ShiftBus1, 1, 1), PinRef(ShiftBus1, 1, 2),
    PinRef(ShiftBus1, 1, 3), PinRef(ShiftBus1, 1, 4), PinRef(ShiftBus1, 1, 5),
    PinRef(ShiftBus1, 1, 6), PinRef(ShiftBus1, 1, 7),
};

class Probe : public OpenSkyhawk::InputBase {
public:
    void configure() override { for (auto& p : gIn) p.configureAsInput(); }
    void poll() override {}
    void forceReport() override {}
} gProbe;

static void printFrames(const char* tag) {
    auto& d = STM32Board::diagSerial();
    d.print(tag);
    d.print(F(" chip0 D7..D0 = "));
    for (int8_t b = 7; b >= 0; b--) d.print(gIn[b].read() ? '1' : '0');
    d.print(F("   chip1 D7..D0 = "));
    for (int8_t b = 15; b >= 8; b--) d.print(gIn[b].read() ? '1' : '0');
    d.println();
}

void setup() {
    STM32Board::setDebug(true);
    PanelGroup::setup();
    STM32Board::diagSerial().println("=== '165 cascade probe: poke inputs, watch bits ===");
}

void loop() {
    PanelGroup::loop();
    static uint16_t lastState = 0xFFFF;
    static uint32_t lastBeat = 0;
    uint16_t state = 0;
    for (uint8_t i = 0; i < 16; i++) state |= (uint16_t)(gIn[i].read() ? 1 : 0) << i;
    if (state != lastState) { lastState = state; printFrames("[change]"); }
    if (millis() - lastBeat >= 2000) { lastBeat = millis(); printFrames("[beat]  "); }
}
