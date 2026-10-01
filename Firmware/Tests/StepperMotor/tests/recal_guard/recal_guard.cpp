// StepperMotor — auto-recal guards (#137)
//
// Two audit findings, both exercised through the sensor-override seam so no sensor or motor is
// needed:
//
//   1. Re-zero only against a home we actually found. An aborted home leaves the needle wherever
//      it stopped; re-zeroing there writes a phantom origin that every later move inherits. With
//      _homed false, an asserted sensor must change nothing.
//   2. Debounce the in-motion sensor. The old code re-zeroed on a single cached read, and
//      recalDebounceMs only spaced recals apart — it filtered nothing. The sensor must now read
//      asserted continuously for sensor.debounceMs, and a glitch mid-window restarts the wait.
//
// Rig: this STM32 alone. No motor, no sensor, no jumper — coils are NC PinRefs and the sensor is
// driven by debugSetSensorOverride().

#include <Arduino.h>
#include <STM32Board.h>
#include <Drivers/StepperMotor/StepperMotor.h>

using namespace OpenSkyhawk;

static constexpr uint16_t DEBOUNCE_MS = 25;

static StepperConfig cfg(uint16_t maxSeek) {
    StepperConfig c = {
        /* stepsPerRev       */ 720,
        /* pattern           */ StepPattern::SWITEC_6STATE,
        /* accel             */ kSwitecDefaultAccel,
        /* accelN            */ kSwitecDefaultAccelN,
        /* home              */ HomeMode::SENSOR,
        /* homeSeekClockwise */ true,
        /* sensor            */ { true, DEBOUNCE_MS, maxSeek },
        /* homePosition      */ 0,
        /* parkPosition      */ 0,
        /* minPos            */ 0,
        /* maxPos            */ 720,
        /* wrap              */ false,
        /* deadband          */ 0,
        /* autoRecal         */ true,
        /* recalDebounceMs   */ 0,
    };
    return c;
}

static const StepperConfig CFG_ABORT = cfg(20);    // sensor never asserts during homing → abort
static const StepperConfig CFG_OK    = cfg(200);

// A home sensor PinRef must be non-NC for the recal path to run at all; PB1 is unwired here and
// never read, because the override short-circuits sensorAsserted().
StepperMotor gAborted(PinRef(), PinRef(), PinRef(), PinRef(), CFG_ABORT, PinRef(PB1));
StepperMotor gHomed  (PinRef(), PinRef(), PinRef(), PinRef(), CFG_OK,    PinRef(PB1));

// Hold the sensor asserted and pump update() until the debounce window has certainly elapsed.
static void pumpAsserted(StepperMotor& m, uint32_t ms) {
    const uint32_t t0 = millis();
    while ((uint32_t)(millis() - t0) < ms) m.update();
}

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== StepperMotor recal_guard ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    // ── 1. an aborted home must not re-zero ──────────────────────────────────
    gAborted.debugSetSensorOverride(1);     // active-LOW + HIGH = idle, so homing aborts
    gAborted.configure();
    gAborted.home();
    check("aborted home: homed() false      ", !gAborted.homed());

    gAborted.debugSetSensorOverride(0);     // sensor now reads asserted
    gAborted.debugSetCurrentStep(123);
    pumpAsserted(gAborted, DEBOUNCE_MS * 3);
    check("aborted home: no phantom re-zero ", gAborted.debugCurrentStep() == 123);

    // ── 2. a successful home allows recal, but only after the debounce window ──
    gHomed.debugSetSensorOverride(0);       // asserted → homes on the first seek
    gHomed.configure();
    gHomed.home();
    check("homed() true after a good home   ", gHomed.homed());

    gHomed.debugSetCurrentStep(321);
    gHomed.update();                        // first asserted read only starts the window
    check("single read does not re-zero     ", gHomed.debugCurrentStep() == 321);

    // A glitch mid-window must restart the wait, not count toward it.
    delay(DEBOUNCE_MS / 2);
    gHomed.debugSetSensorOverride(1);       // sensor drops out
    gHomed.update();
    gHomed.debugSetSensorOverride(0);       // and comes back
    gHomed.update();
    delay(DEBOUNCE_MS / 2);
    gHomed.update();                        // only ~half a window since the restart
    check("glitch restarts the window       ", gHomed.debugCurrentStep() == 321);

    pumpAsserted(gHomed, DEBOUNCE_MS * 2);  // now hold it steady past the window
    check("steady assert re-zeros to home   ", gHomed.debugCurrentStep() == 0);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
