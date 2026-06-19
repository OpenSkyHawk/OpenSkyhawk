// StepperMotor — bringup sweep (bench: real X27 on 4 GPIO at 3.3 V, no driver)
//
// Homes into the mechanical stop, then sweeps the needle end-to-end every few seconds.
// Eyeball: smooth acceleration, no missed steps, clean stop at each end.
//
// Wiring: 4 coil pins below → X27 / VID-29 / BKA-30 air-core stepper. If the needle
// runs backwards, swap any two coil pins.

#include <Arduino.h>
#include <STM32Board.h>
#include <Drivers/StepperMotor/StepperMotor.h>

using namespace OpenSkyhawk;

static constexpr int16_t SWEEP_LO = 30;
static constexpr int16_t SWEEP_HI = 600;

static const StepperConfig CFG = {
    /* stepsPerRev       */ 720,
    /* pattern           */ StepPattern::SWITEC_6STATE,
    /* accel             */ kSwitecDefaultAccel,
    /* accelN            */ kSwitecDefaultAccelN,
    /* home              */ HomeMode::STALL,
    /* homeSeekClockwise */ false,
    /* sensor            */ { false, 0, 0 },
    /* homePosition      */ 0,
    /* parkPosition      */ SWEEP_LO,
    /* minPos            */ 0,
    /* maxPos            */ 720,
    /* wrap              */ false,
    /* deadband          */ 1,
    /* autoRecal         */ false,
    /* recalDebounceMs   */ 0,
};

StepperMotor gMotor(PinRef(PA0), PinRef(PA1), PinRef(PA4), PinRef(PA5), CFG);

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== StepperMotor bringup (bench) ===");
    gMotor.configure();
    gMotor.home();
    STM32Board::diagSerial().println(gMotor.homed() ? "homed; sweeping" : "HOME FAILED");
}

void loop() {
    static uint32_t lastFlip = 0;
    static bool high = false;
    uint32_t now = millis();
    if (now - lastFlip >= 3000) {
        lastFlip = now;
        high = !high;
        gMotor.moveTo(high ? SWEEP_HI : SWEEP_LO);
    }
    gMotor.update();
}
