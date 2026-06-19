// StepperMotor — step pattern (tier 1: deterministic)
//
// Both the 6-state air-core sequence and the 4-state full-step sequence advance the
// position identically (the pattern only changes which coils energise). Exercises both
// state-count modulos to the same target.

#include <Arduino.h>
#include <STM32Board.h>
#include <Drivers/StepperMotor/StepperMotor.h>

using namespace OpenSkyhawk;

static StepperConfig cfg(StepPattern pattern) {
    StepperConfig c = {
        /* stepsPerRev       */ 720,
        /* pattern           */ pattern,
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
        /* deadband          */ 0,
        /* autoRecal         */ false,
        /* recalDebounceMs   */ 0,
    };
    return c;
}

static const StepperConfig CFG_6 = cfg(StepPattern::SWITEC_6STATE);
static const StepperConfig CFG_4 = cfg(StepPattern::FULL_4STATE);

StepperMotor g6(PinRef(), PinRef(), PinRef(), PinRef(), CFG_6);
StepperMotor g4(PinRef(), PinRef(), PinRef(), PinRef(), CFG_4);

static void runToStop(StepperMotor& m) {
    for (uint32_t i = 0; i < 5000 && !m.debugStopped(); i++) m.debugAdvance();
}

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== StepperMotor step_pattern ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    g6.configure();
    g4.configure();

    g6.moveTo(100); runToStop(g6);
    check("6-state reaches target (100)", g6.position() == 100 && g6.debugStopped());

    g4.moveTo(100); runToStop(g4);
    check("4-state reaches target (100)", g4.position() == 100 && g4.debugStopped());

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
