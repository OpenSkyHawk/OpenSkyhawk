// ShiftBus — bench gate 5: shared-bus non-interference, everything at once.
//
// '595: stepper sweeping 0<->900 (coils Q1-Q4, nSLEEP Q5) + LED 1 Hz (Q6).
// '165: REL encoder on D4/D5 (E/F), ISR-sampled at SHIFTBUS_ISR_HZ, 25 ms loop stall
// active. Spin the encoder while the motor sweeps: detent totals must stay exact,
// sweeps must stay 791 ms, LED must stay clean. Any cross-talk between the '165 read
// path and the '595 write path on the shared SCK = FAIL.

#include <Arduino.h>
#include <STM32Board.h>
#include <PanelGroup.h>
#include <Drivers/StepperMotor/StepperMotor.h>
#include <Inputs/RotaryEncoder/RotaryEncoder.h>

using namespace OpenSkyhawk;

const StepperConfig kCfg = makeX27Config(0, 0, 0, 900);

StepperMotor gMotor(PinRef(ShiftBus1, 0, 1), PinRef(ShiftBus1, 0, 2),
                    PinRef(ShiftBus1, 0, 3), PinRef(ShiftBus1, 0, 4),
                    kCfg, PinRef(), PinRef(ShiftBus1, 0, 5));
PinRef gLed(ShiftBus1, 0, 6);

RotaryEncoder gEnc(0x3B01, PinRef(ShiftBus1, 0, 4 + 0), PinRef(ShiftBus1, 0, 5 + 0),
                   EncoderStepsPerDetent::Four, EncoderMode::Rel);
// NOTE: encoder chip index differs from the '595! Inputs live on the '165 chain —
// chip 0 of the INPUT chain, bits D4/D5 — independent numbering from output chip 0.

class Rig : public OutputBase {
public:
    void configure() override { gMotor.configure(); gLed.configureAsOutput(); }
    void onControlPacket(uint16_t, uint16_t) override {}
    void update() override {
        gMotor.update();
        const uint32_t now = millis();
        if (_sweeping && gMotor.position() == (_toMax ? 900 : 0)) {
            _sweeping = false;
            const uint32_t ms = now - _t0;
            auto& d = STM32Board::diagSerial();
            d.print(F("[gate5] sweep ")); d.print(ms); d.println(F(" ms"));
        }
        if (_homed && now - _last >= 3000) {
            _last = now; _toMax = !_toMax;
            gMotor.moveTo(_toMax ? 900 : 0);
            _t0 = now; _sweeping = true;
        }
        if (now - _blink >= 1000) { _blink = now; _on = !_on; gLed.write(_on); }
    }
    void kickoff() { gMotor.home(); _homed = true; _last = millis(); }
private:
    bool _homed=false,_toMax=false,_on=false,_sweeping=false;
    uint32_t _last=0,_blink=0,_t0=0;
};
Rig gRig;

void setup() {
    STM32Board::setDebug(true);
    PanelGroup::setup();
    auto& d = STM32Board::diagSerial();
    d.println("=== ShiftBus bench gate 5: stepper + LED + ISR encoder, one bus ===");
#ifdef SHIFTBUS_ISR_HZ
    d.print(F("ISR @")); d.print(SHIFTBUS_ISR_HZ); d.println(F(" Hz"));
#endif
    gRig.kickoff();
}

void loop() {
    PanelGroup::loop();
    // No artificial stall here: a blocked loop stutters stepper step-timing on ANY
    // backend (that finding is logged for grouping guidance). Gate 5 isolates BUS
    // interference: encoder spin vs sweep timing vs LED on one shared SCK.
    static uint32_t print = 0;
    if (millis() - print >= 2000) {
        print = millis();
        auto& d = STM32Board::diagSerial();
        d.print(F("[gate5] detents=")); d.println((int32_t)gEnc.netDetents());
    }
}
