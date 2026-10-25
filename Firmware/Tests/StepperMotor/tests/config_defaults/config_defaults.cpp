// StepperMotor — config_defaults (tier 1: deterministic)
//
// StepperConfig carries the X27-family defaults on its fields (#295), so a sketch names only what
// differs. Checks that makeX27Config() is exactly those defaults plus its five arguments, and that
// a by-name initialiser changes only the fields it names. Compiling the by-name form is half the
// test: it must stay valid as fields are appended.

#include <Arduino.h>
#include <STM32Board.h>
#include <Drivers/StepperMotor/StepperMotor.h>

using namespace OpenSkyhawk;

static const StepperConfig DEFAULTS = {};
static const StepperConfig X27      = makeX27Config(/*home*/ 0, /*park*/ 10, /*min*/ 0, /*max*/ 810);
static const StepperConfig BKA30    = { .maxPos = 960, .rangeSteps = 960 };   // 320° BKA-30

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== StepperMotor config_defaults ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    check("defaults: 1080 steps/rev, 6-state, Switec accel",
          DEFAULTS.stepsPerRev == 1080 && DEFAULTS.pattern == StepPattern::SWITEC_6STATE
          && DEFAULTS.accel == kSwitecDefaultAccel && DEFAULTS.accelN == kSwitecDefaultAccelN);
    check("defaults: STALL home, 945 range, deadband 1",
          DEFAULTS.home == HomeMode::STALL && DEFAULTS.rangeSteps == 945 && DEFAULTS.deadband == 1
          && DEFAULTS.minPos == 0 && DEFAULTS.maxPos == 945);
    check("defaults: sensor active-low, 5 ms, 2000 steps",
          DEFAULTS.sensor.activeLow && DEFAULTS.sensor.debounceMs == 5
          && DEFAULTS.sensor.maxSeekSteps == 2000);

    check("makeX27Config: its arguments",
          X27.homePosition == 0 && X27.parkPosition == 10 && X27.minPos == 0 && X27.maxPos == 810
          && X27.home == HomeMode::STALL);
    check("makeX27Config: everything else is the default",
          X27.stepsPerRev == DEFAULTS.stepsPerRev && X27.rangeSteps == DEFAULTS.rangeSteps
          && X27.deadband == DEFAULTS.deadband && X27.homeStepUs == DEFAULTS.homeStepUs
          && X27.accel == DEFAULTS.accel && !X27.wrap && !X27.autoRecal);

    check("by name: only the named fields change",
          BKA30.maxPos == 960 && BKA30.rangeSteps == 960
          && BKA30.stepsPerRev == DEFAULTS.stepsPerRev && BKA30.minPos == DEFAULTS.minPos
          && BKA30.deadband == DEFAULTS.deadband);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
