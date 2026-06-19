// StepperMotor — coils on an MCP23017 expander (bench)
//
// Same sweep as bringup, but the 4 coils are driven through an MCP23017 (GPA0..GPA3)
// instead of native GPIO — proving the PinRef path drives a stepper over I2C. Step rate
// is capped by the per-pin I2C write (~one transaction per coil), so keep sweeps moderate.
//
// Hardware: STM32 + MCP23017 @ 0x20 on I2C1 (PB8=SCL, PB9=SDA); X27 coils on GPA0..GPA3.

#include <Arduino.h>
#include <STM32Board.h>
#include <Wire.h>
#include <MCP23017.h>
#include <PanelGroup.h>
#include <Drivers/StepperMotor/StepperMotor.h>

using namespace OpenSkyhawk;

MCP23017 gExpander(0x20, Wire);

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

StepperMotor gMotor(PinRef(gExpander, PORT_A, 0), PinRef(gExpander, PORT_A, 1),
                    PinRef(gExpander, PORT_A, 2), PinRef(gExpander, PORT_A, 3), CFG);

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== StepperMotor mcp23017 (bench) ===");

    Wire.setSDA(PB9);
    Wire.setSCL(PB8);
    Wire.begin();
    PanelGroup::registerExpander(gExpander);   // polling-fallback mode
    PanelGroup::setup();                        // inits the expander

    gMotor.configure();                         // coils → MCP outputs
    gMotor.home();
    STM32Board::diagSerial().println(gMotor.homed() ? "homed; sweeping over I2C" : "HOME FAILED");
}

void loop() {
    PanelGroup::loop();   // services the expander
    static uint32_t lastFlip = 0;
    static bool high = false;
    uint32_t now = millis();
    if (now - lastFlip >= 4000) {
        lastFlip = now;
        high = !high;
        gMotor.moveTo(high ? 600 : 30);
    }
    gMotor.update();
}
