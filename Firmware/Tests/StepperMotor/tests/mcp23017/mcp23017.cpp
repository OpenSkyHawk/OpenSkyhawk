// StepperMotor — coils on an MCP23017 expander (bench)
//
// Same sweep as bringup, but the 4 coils are driven through an MCP23017 (GPB0..GPB3)
// instead of native GPIO — proving the PinRef path drives a stepper over I2C. Step rate
// is capped by the per-pin I2C write (~one transaction per coil), so keep sweeps moderate.
//
// Hardware: STM32 + MCP23017 @ 0x20 on I2C1 (PB8=SCL, PB9=SDA); X27 coils on GPB0..GPB3.

#include <Arduino.h>
#include <STM32Board.h>
#include <Wire.h>
#include <MCP23017.h>
#include <PanelGroup.h>
#include <Drivers/StepperMotor/StepperMotor.h>

using namespace OpenSkyhawk;

MCP23017 gExpander(0x20, Wire);

static const StepperConfig CFG = {
    /* stepsPerRev       */ 1080,   // full revolution (datasheet)
    /* pattern           */ StepPattern::SWITEC_6STATE,
    /* accel             */ kSwitecDefaultAccel,
    /* accelN            */ kSwitecDefaultAccelN,
    /* home              */ HomeMode::STALL,
    /* homeSeekClockwise */ false,
    /* sensor            */ { false, 0, 0 },
    /* homePosition      */ 0,
    /* parkPosition      */ 30,
    /* minPos            */ 0,
    /* maxPos            */ 960,
    /* wrap              */ false,
    /* deadband          */ 1,
    /* autoRecal         */ false,
    /* recalDebounceMs   */ 0,
    /* rangeSteps        */ 960,    // stop-to-stop — STALL home drives this
    /* homeStepUs        */ 0,      // 0 → library default 2000µs
};

StepperMotor gMotor(PinRef(gExpander, PORT_B, 0), PinRef(gExpander, PORT_B, 1),
                    PinRef(gExpander, PORT_B, 2), PinRef(gExpander, PORT_B, 3), CFG);

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== StepperMotor mcp23017 (bench) ===");

    Wire.setSDA(PB9);
    Wire.setSCL(PB8);
    Wire.begin();
    Wire.setClock(400000);   // 400kHz — STM32F103 I2C ceiling (no fast-mode-plus on F1)
    PanelGroup::registerExpander(gExpander);   // polling-fallback mode
    PanelGroup::setup();                        // inits the expander

    gMotor.configure();                         // coils → MCP outputs
    gMotor.home();
    STM32Board::diagSerial().println(gMotor.homed() ? "homed; sweeping over I2C" : "HOME FAILED");
}

// Timed sweep — reports achieved step rate so the 400kHz vs 100kHz I2C cap is measurable.
static void sweepTo(int16_t target) {
    int32_t start = gMotor.position();
    uint32_t t0 = millis();
    gMotor.moveTo(target);
    while (!gMotor.debugStopped() && millis() - t0 < 8000) { PanelGroup::loop(); gMotor.update(); }
    uint32_t ms = millis() - t0;
    int32_t moved = gMotor.position() - start; if (moved < 0) moved = -moved;
    auto& s = STM32Board::diagSerial();
    s.print("  -> ");      s.print(target);
    s.print(" : ");        s.print(moved);
    s.print(" steps in "); s.print(ms);
    s.print(" ms (");      s.print(ms ? moved * 1000L / (int32_t)ms : 0);
    s.println(" steps/s @400kHz)");
}

void loop() {
    sweepTo(600);
    delay(500);
    sweepTo(30);
    delay(500);
}
