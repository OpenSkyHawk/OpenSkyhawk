// StepperMotor — STALL homing (tier 1: deterministic, no motor)
//
// Drives a full revolution into the (notional) mechanical stop, calls that homePosition,
// then parks. Coils are NC — only the position bookkeeping runs.

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
    /* parkPosition      */ 50,
    /* minPos            */ 0,
    /* maxPos            */ 720,
    /* wrap              */ false,
    /* deadband          */ 0,
    /* autoRecal         */ false,
    /* recalDebounceMs   */ 0,
};

StepperMotor gMotor(PinRef(), PinRef(), PinRef(), PinRef(), CFG);

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== StepperMotor home_stall ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    gMotor.configure();
    check("not homed before home()", !gMotor.homed());

    gMotor.home();
    check("homed() true after STALL home", gMotor.homed());
    check("parked at parkPosition (50)", gMotor.position() == 50);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
