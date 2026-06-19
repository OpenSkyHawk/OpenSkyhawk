// StepperMotor — SENSOR homing (tier 1: deterministic via the sensor-override seam)
//
// Verifies the sensor-type-agnostic homing logic without a real sensor:
//   - active-LOW and active-HIGH polarity both resolve correctly
//   - a never-asserting (mis-wired) sensor aborts after maxSeekSteps — no hang
//   - an asserting sensor homes + parks, homed() == true
// The override forces the debounced read, so any switch/reed/hall/opto behaves identically.

#include <Arduino.h>
#include <STM32Board.h>
#include <Drivers/StepperMotor/StepperMotor.h>

using namespace OpenSkyhawk;

static StepperConfig cfg(bool activeLow, uint16_t maxSeek, int16_t park) {
    StepperConfig c = {
        /* stepsPerRev       */ 720,
        /* pattern           */ StepPattern::SWITEC_6STATE,
        /* accel             */ kSwitecDefaultAccel,
        /* accelN            */ kSwitecDefaultAccelN,
        /* home              */ HomeMode::SENSOR,
        /* homeSeekClockwise */ true,
        /* sensor            */ { activeLow, 0, maxSeek },
        /* homePosition      */ 0,
        /* parkPosition      */ park,
        /* minPos            */ 0,
        /* maxPos            */ 720,
        /* wrap              */ false,
        /* deadband          */ 0,
        /* autoRecal         */ false,
        /* recalDebounceMs   */ 0,
    };
    return c;
}

static const StepperConfig CFG_LOW  = cfg(true,  50,  0);
static const StepperConfig CFG_HIGH = cfg(false, 50,  0);
static const StepperConfig CFG_OK   = cfg(true,  200, 20);

StepperMotor gLow (PinRef(), PinRef(), PinRef(), PinRef(), CFG_LOW);
StepperMotor gHigh(PinRef(), PinRef(), PinRef(), PinRef(), CFG_HIGH);
StepperMotor gOk  (PinRef(), PinRef(), PinRef(), PinRef(), CFG_OK);

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== StepperMotor home_sensor ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    // ── polarity ──────────────────────────────────────────────────────────────
    gLow.debugSetSensorOverride(0);   // raw LOW
    check("active-LOW: LOW reads asserted",   gLow.debugSensorAsserted());
    gLow.debugSetSensorOverride(1);   // raw HIGH
    check("active-LOW: HIGH reads idle",     !gLow.debugSensorAsserted());
    gHigh.debugSetSensorOverride(1);  // raw HIGH
    check("active-HIGH: HIGH reads asserted", gHigh.debugSensorAsserted());
    gHigh.debugSetSensorOverride(0);  // raw LOW
    check("active-HIGH: LOW reads idle",     !gHigh.debugSensorAsserted());

    // ── abort: sensor never asserts → seek gives up after maxSeekSteps ──────────
    gLow.debugSetSensorOverride(1);   // active-LOW + HIGH = never asserted
    gLow.configure();
    gLow.home();
    check("never-asserting sensor aborts (no hang)", !gLow.homed());

    // ── success: sensor asserted → homes + parks ────────────────────────────────
    gOk.debugSetSensorOverride(0);    // active-LOW + LOW = asserted
    gOk.configure();
    gOk.home();
    check("asserting sensor homes()", gOk.homed());
    check("parks at parkPosition (20)", gOk.position() == 20);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
