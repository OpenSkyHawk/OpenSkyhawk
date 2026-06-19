// StepperMotor — accuracy / speed-envelope sweep (bench)
//
// Homes to the stall reference, then repeatedly sweeps end-to-end, reporting the achieved
// step rate per leg (the GPIO vs MCP23017 envelope) and re-homing every few cycles to expose
// accumulated missed steps. To push the envelope, supply a faster accel table (shorter last
// delayUs) and watch for the needle failing to reach the ends or the re-home drifting.

#include <Arduino.h>
#include <STM32Board.h>
#include <Drivers/StepperMotor/StepperMotor.h>

using namespace OpenSkyhawk;

static constexpr int16_t LO = 20;
static constexpr int16_t HI = 680;

static const StepperConfig CFG = {
    /* stepsPerRev       */ 720,
    /* pattern           */ StepPattern::SWITEC_6STATE,
    /* accel             */ kSwitecDefaultAccel,
    /* accelN            */ kSwitecDefaultAccelN,
    /* home              */ HomeMode::STALL,
    /* homeSeekClockwise */ false,
    /* sensor            */ { false, 0, 0 },
    /* homePosition      */ 0,
    /* parkPosition      */ LO,
    /* minPos            */ 0,
    /* maxPos            */ 720,
    /* wrap              */ false,
    /* deadband          */ 0,
    /* autoRecal         */ false,
    /* recalDebounceMs   */ 0,
};

StepperMotor gMotor(PinRef(PA0), PinRef(PA1), PinRef(PA4), PinRef(PA5), CFG);

static void sweepTo(int16_t target) {
    int32_t startPos = gMotor.position();
    uint32_t t0 = millis();
    gMotor.moveTo(target);
    while (!gMotor.debugStopped() && (millis() - t0) < 5000) gMotor.update();
    uint32_t ms = millis() - t0;
    int32_t moved = gMotor.position() - startPos; if (moved < 0) moved = -moved;
    STM32Board::diagSerial().print("  -> ");        STM32Board::diagSerial().print(target);
    STM32Board::diagSerial().print(" : ");          STM32Board::diagSerial().print(moved);
    STM32Board::diagSerial().print(" steps in ");   STM32Board::diagSerial().print(ms);
    STM32Board::diagSerial().print(" ms (");
    STM32Board::diagSerial().print(ms ? (moved * 1000L / (int32_t)ms) : 0);
    STM32Board::diagSerial().println(" steps/s)");
}

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== StepperMotor accuracy_sweep (bench) ===");
    gMotor.configure();
    gMotor.home();
}

void loop() {
    static uint8_t cycle = 0;
    sweepTo(HI);
    sweepTo(LO);
    if (++cycle >= 10) {
        cycle = 0;
        gMotor.home();
        STM32Board::diagSerial().print("re-homed; reported pos = ");
        STM32Board::diagSerial().println(gMotor.position());
    }
}
