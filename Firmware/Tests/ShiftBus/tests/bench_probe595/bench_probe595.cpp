// ShiftBus — bench probe: walking bit across a 2-chip '595 cascade, 1 s per output.
// Serial prints "chip C bit B"; watch the LED (chip 1, Q0 per the wiring diagram) light
// when its index comes around — proves SCK/MOSI/LATCH wiring AND the QH'->DS cascade hop.

#include <Arduino.h>
#include <STM32Board.h>
#include <PanelGroup.h>

PinRef gOut[16] = {
    PinRef(ShiftBus1, 0, 0), PinRef(ShiftBus1, 0, 1), PinRef(ShiftBus1, 0, 2),
    PinRef(ShiftBus1, 0, 3), PinRef(ShiftBus1, 0, 4), PinRef(ShiftBus1, 0, 5),
    PinRef(ShiftBus1, 0, 6), PinRef(ShiftBus1, 0, 7),
    PinRef(ShiftBus1, 1, 0), PinRef(ShiftBus1, 1, 1), PinRef(ShiftBus1, 1, 2),
    PinRef(ShiftBus1, 1, 3), PinRef(ShiftBus1, 1, 4), PinRef(ShiftBus1, 1, 5),
    PinRef(ShiftBus1, 1, 6), PinRef(ShiftBus1, 1, 7),
};

// Minimal OutputBase shim so PanelGroup configures the pins (activates the bus).
class Probe : public OpenSkyhawk::OutputBase {
public:
    void configure() override { for (auto& p : gOut) p.configureAsOutput(); }
    void onControlPacket(uint16_t, uint16_t) override {}
} gProbe;

void setup() {
    STM32Board::setDebug(true);
    PanelGroup::setup();
    STM32Board::diagSerial().println("=== '595 cascade walking-bit probe: chip 0..1, Q0..Q7, 1 s each ===");
}

void loop() {
    PanelGroup::loop();
    static uint32_t last = 0;
    static uint8_t  idx  = 0;
    if (millis() - last >= 1000) {
        last = millis();
        for (uint8_t i = 0; i < 16; i++) gOut[i].write(i == idx);
        auto& d = STM32Board::diagSerial();
        d.print(F("[probe] chip ")); d.print(idx / 8);
        d.print(F(" bit "));         d.print(idx % 8);
        d.println(F(" HIGH"));
        idx = (uint8_t)((idx + 1) & 15);
    }
}
