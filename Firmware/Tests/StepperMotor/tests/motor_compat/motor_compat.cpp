// StepperMotor — one-profile motor compatibility (bench)
//
// X27.589 / VID-29 / BKA-30 are the same air-core family and run on the ONE 6-state
// profile. Flash this, wire each motor in turn to PA0/PA1/PA4/PA5, and confirm the same
// smooth sweep. If a motor runs reversed, swap any two coil pins (the AHN "BKA-30 reorder")
// — no config change, no separate profile.

#include <Arduino.h>
#include <STM32Board.h>
#include <Drivers/StepperMotor/StepperMotor.h>

using namespace OpenSkyhawk;

static const StepperConfig CFG = {
    /* stepsPerRev       */ 720,
    /* pattern           */ StepPattern::SWITEC_6STATE,
    /* accel             */ kSwitecDefaultAccel,
    /* accelN            */ kSwitecDefaultAccelN,
    /* home              */ HomeMode::STALL,
    /* homeSeekClockwise */ false,
    /* sensor            */ { false, 0, 0 },
    /* homePosition      */ 0,
    /* parkPosition      */ 30,
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
    STM32Board::diagSerial().println("=== StepperMotor motor_compat (bench) ===");
    STM32Board::diagSerial().println("Same 6-state profile for X27 / VID-29 / BKA-30. Swap 2 pins if reversed.");
    gMotor.configure();
    gMotor.home();
}

void loop() {
    static uint32_t lastFlip = 0;
    static bool high = false;
    uint32_t now = millis();
    if (now - lastFlip >= 2500) {
        lastFlip = now;
        high = !high;
        gMotor.moveTo(high ? 600 : 30);
    }
    gMotor.update();
}
