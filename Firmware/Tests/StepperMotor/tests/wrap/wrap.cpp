// StepperMotor — wrap / continuous-rotation shortest path (tier 1: deterministic)
//
// For a continuous gauge, moveTo() picks the shortest signed path around stepsPerRev,
// and position() reports the wrapped 0..stepsPerRev value.

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
    /* parkPosition      */ 0,
    /* minPos            */ 0,
    /* maxPos            */ 720,
    /* wrap              */ true,
    /* deadband          */ 0,
    /* autoRecal         */ false,
    /* recalDebounceMs   */ 0,
};

StepperMotor gMotor(PinRef(), PinRef(), PinRef(), PinRef(), CFG);

static void runToStop() {
    for (uint32_t i = 0; i < 5000 && !gMotor.debugStopped(); i++) gMotor.debugAdvance();
}

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== StepperMotor wrap ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    gMotor.configure();   // currentStep 0

    // 0 → 700: short way is backwards 20 steps (700 ≡ -20), not forward 700
    gMotor.moveTo(700);
    check("target takes short path (-20)", gMotor.debugTargetStep() == -20);
    runToStop();
    check("position wraps to 700", gMotor.position() == 700);
    check("currentStep is -20", gMotor.debugCurrentStep() == -20);

    // 700 → 10: short way is forward 30 steps (700 → 730 ≡ 10)
    gMotor.moveTo(10);
    check("target takes short path (+30 → 10)", gMotor.debugTargetStep() == 10);
    runToStop();
    check("position wraps to 10", gMotor.position() == 10);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
