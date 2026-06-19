// StepperMotor — deadband (tier 1: deterministic)
//
// Target changes within `deadband` steps of the current target are ignored (anti-jitter);
// larger changes retarget.

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
    /* wrap              */ false,
    /* deadband          */ 5,
    /* autoRecal         */ false,
    /* recalDebounceMs   */ 0,
};

StepperMotor gMotor(PinRef(), PinRef(), PinRef(), PinRef(), CFG);

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== StepperMotor deadband ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    gMotor.configure();   // target starts at 0

    gMotor.moveTo(3);     // |3-0| = 3 <= 5 → ignored
    check("within deadband ignored (target stays 0)", gMotor.debugTargetStep() == 0);

    gMotor.moveTo(10);    // |10-0| = 10 > 5 → retarget
    check("beyond deadband retargets (10)", gMotor.debugTargetStep() == 10);

    gMotor.moveTo(12);    // |12-10| = 2 <= 5 → ignored
    check("small change ignored (target stays 10)", gMotor.debugTargetStep() == 10);

    gMotor.moveTo(20);    // |20-10| = 10 > 5 → retarget
    check("larger change retargets (20)", gMotor.debugTargetStep() == 20);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
