// ShiftBus — bench gates 2+3(+4): 74HC595 → DRV8833 → X27 stepper, plus direct-drive LED
//
// '595 chip 0 map (the APN-153 node shape): Q0–Q3 → DRV8833 AIN1/AIN2/BIN1/BIN2,
// Q4 → DRV8833 nSLEEP (10 k pull-down), Q5 → LED + series R (direct drive — no MOSFET;
// gate 3 also observes brightness/cleanliness at the '595 pin budget).
//
// The stepper sweeps continuously between two targets through the production path:
// StepperMotor coils are SR PinRefs; each step = writeDeferred ×4 + flushExpanderWrites()
// = ONE SPI burst. The LED toggles at 1 Hz through the loop-rate transfer.
//
// Gate 2 PASS: smooth 6-state drive, sweep rate visibly motor-limited (~600 °/s), no stalls.
// Gate 3 PASS: LED clean on/off, no shimmer from stepper traffic on the same chip.
// Gate 4 OBSERVE: power-on twitch/flash before setup() zeroes the frame (cosmetic, expected).
//
// Rig: 74HC595 on the standard pins (SCK=PB3 MOSI=PB5 LATCH=PB9), DRV8833 module
// (bench module: jumper EEP→VCC is NOT needed here — nSLEEP is driven by Q4), X27 gauge
// motor, LED+R on Q5. CAN bus to the PanelBridge.

#include <Arduino.h>
#include <STM32Board.h>
#include <PanelGroup.h>
#include <Drivers/StepperMotor/StepperMotor.h>

using namespace OpenSkyhawk;

const StepperConfig kCfg = makeX27Config(/*homePosition=*/0, /*parkPosition=*/0,
                                         /*minPos=*/0, /*maxPos=*/900);

StepperMotor gMotor(PinRef(ShiftBus1, 0, 0), PinRef(ShiftBus1, 0, 1),
                    PinRef(ShiftBus1, 0, 2), PinRef(ShiftBus1, 0, 3),
                    kCfg,
                    /*homeSense=*/PinRef(),
                    /*sleepEn=*/PinRef(ShiftBus1, 0, 4));

PinRef gLed(ShiftBus1, 0, 5);

// StepperMotor is a MotorDriver, not an OutputBase — a thin shim gives it loop updates
// and drives the sweep, standing in for the NeedleGauge that owns it in production.
class SweepRig : public OutputBase {
public:
    void configure() override {
        gMotor.configure();
        gLed.configureAsOutput();
    }
    void onControlPacket(uint16_t, uint16_t) override {}
    void update() override {
        gMotor.update();
        const uint32_t now = millis();
        if (_homed && now - _lastSweep >= 3000) {   // 900 steps = 300° well inside 3 s
            _lastSweep = now;
            _toMax = !_toMax;
            gMotor.moveTo(_toMax ? 900 : 0);
            auto& d = STM32Board::diagSerial();
            d.print(F("[gate2] sweep -> ")); d.println(_toMax ? 900 : 0);
        }
        if (now - _lastBlink >= 1000) {             // gate 3: 1 Hz LED via loop transfer
            _lastBlink = now;
            _ledOn = !_ledOn;
            gLed.write(_ledOn);
        }
    }
    void kickoff() {
        STM32Board::diagSerial().println(F("[gate2] homing (STALL sweep)…"));
        gMotor.home();
        _homed = true;
        _lastSweep = millis();
    }
private:
    bool     _homed = false, _toMax = false, _ledOn = false;
    uint32_t _lastSweep = 0, _lastBlink = 0;
};

SweepRig gRig;

void setup() {
    STM32Board::setDebug(true);
    PanelGroup::setup();
    STM32Board::diagSerial().println("=== ShiftBus bench gates 2+3: '595 stepper + LED ===");
    gRig.kickoff();
}

void loop() {
    PanelGroup::loop();
}
