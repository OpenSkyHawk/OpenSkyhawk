// ShiftBus — bench probe: walking bit across all 8 '595 outputs, 1 s each.
// Watch the LED (and a multimeter on any output): serial prints the active bit.
// Calibrates the physical output→bit map and proves SCK/MOSI/LATCH wiring end-to-end.

#include <Arduino.h>
#include <STM32Board.h>
#include <PanelGroup.h>

PinRef gOut[8] = {
    PinRef(ShiftBus1, 0, 0), PinRef(ShiftBus1, 0, 1), PinRef(ShiftBus1, 0, 2),
    PinRef(ShiftBus1, 0, 3), PinRef(ShiftBus1, 0, 4), PinRef(ShiftBus1, 0, 5),
    PinRef(ShiftBus1, 0, 6), PinRef(ShiftBus1, 0, 7),
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
    STM32Board::diagSerial().println("=== '595 walking-bit probe: Q0..Q7, 1 s each ===");
}

void loop() {
    PanelGroup::loop();
    static uint32_t last = 0;
    static uint8_t  bit  = 0;
    if (millis() - last >= 1000) {
        last = millis();
        for (uint8_t i = 0; i < 8; i++) gOut[i].write(i == bit);
        auto& d = STM32Board::diagSerial();
        d.print(F("[probe] bit ")); d.print(bit);
        d.println(F(" HIGH (Q of that index)"));
        bit = (uint8_t)((bit + 1) & 7);
    }
}
