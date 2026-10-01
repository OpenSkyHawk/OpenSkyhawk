// StepperMotor — coil placement check (#137)
//
// writeIO() caches the four coil bits with writeDeferred() and lets
// PanelGroup::flushExpanderWrites() push ONE writePort() per dirty port, so the whole coil pattern
// changes in a single I2C transaction. Coils split across two MCP ports — or two chips — flush as
// two transactions, leaving the pattern half-applied between them: shoot-through and lost steps.
//
// configure() now refuses such a motor and says so, rather than driving it erratically. Verified
// here both ways: a split placement is rejected, one port is accepted, and a rejected motor stays
// inert when update() is pumped.
//
// Rig: this STM32 alone. The MCP23017 objects are constructed but never talked to — the check only
// compares PinRef backends, so no expander hardware (and no I2C traffic) is involved.

#include <Arduino.h>
#include <Wire.h>
#include <MCP23017.h>
#include <STM32Board.h>
#include <Drivers/StepperMotor/StepperMotor.h>

using namespace OpenSkyhawk;

static const StepperConfig CFG = {
    /* stepsPerRev       */ 720,
    /* pattern           */ StepPattern::SWITEC_6STATE,
    /* accel             */ kSwitecDefaultAccel,
    /* accelN            */ kSwitecDefaultAccelN,
    /* home              */ HomeMode::STALL,
    /* homeSeekClockwise */ true,
    /* sensor            */ { true, 0, 50 },
    /* homePosition      */ 0,
    /* parkPosition      */ 0,
    /* minPos            */ 0,
    /* maxPos            */ 720,
    /* wrap              */ false,
    /* deadband          */ 0,
    /* autoRecal         */ false,
    /* recalDebounceMs   */ 0,
};

MCP23017 gChipA(0x20, Wire);
MCP23017 gChipB(0x21, Wire);

static constexpr uint8_t PORT_A_ = 0;
static constexpr uint8_t PORT_B_ = 1;

// Four coils on one chip, one port — the supported placement.
StepperMotor gOnePort(PinRef(gChipA, PORT_A_, 0), PinRef(gChipA, PORT_A_, 1),
                      PinRef(gChipA, PORT_A_, 2), PinRef(gChipA, PORT_A_, 3), CFG);

// Last coil on the other port of the same chip — two transactions per step.
StepperMotor gSplitPort(PinRef(gChipA, PORT_A_, 0), PinRef(gChipA, PORT_A_, 1),
                        PinRef(gChipA, PORT_A_, 2), PinRef(gChipA, PORT_B_, 0), CFG);

// Last coil on a different chip — also two transactions.
StepperMotor gSplitChip(PinRef(gChipA, PORT_A_, 0), PinRef(gChipA, PORT_A_, 1),
                        PinRef(gChipA, PORT_A_, 2), PinRef(gChipB, PORT_A_, 0), CFG);

// All four on direct GPIO: written immediately, never deferred, so placement cannot split.
StepperMotor gGpio(PinRef(PB12), PinRef(PB13), PinRef(PB14), PinRef(PB15), CFG);

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== StepperMotor coil_port_check ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    gOnePort.configure();
    gSplitPort.configure();
    gSplitChip.configure();
    gGpio.configure();

    check("one chip, one port: accepted    ", gOnePort.debugCoilsOk());
    check("split across ports: refused     ", !gSplitPort.debugCoilsOk());
    check("split across chips: refused     ", !gSplitChip.debugCoilsOk());
    check("four direct GPIO coils: accepted", gGpio.debugCoilsOk());

    // A refused motor must stay inert: no stepping, no position drift.
    gSplitPort.moveTo(100);
    const int32_t before = gSplitPort.debugCurrentStep();
    for (uint8_t i = 0; i < 50; i++) gSplitPort.update();
    check("refused motor does not step     ", gSplitPort.debugCurrentStep() == before);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
