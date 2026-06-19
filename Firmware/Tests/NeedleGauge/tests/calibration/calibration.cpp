// NeedleGauge — piecewise (non-linear) calibration (tier 1: deterministic)
//
// A curve (curveIn → curveOut) interpolates non-linear dials (airspeed, VVI). Breakpoints
// return exactly; intermediate values interpolate linearly within their segment.

#include <Arduino.h>
#include <STM32Board.h>
#include <Drivers/StepperMotor/StepperMotor.h>
#include <Outputs/NeedleGauge/NeedleGauge.h>

using namespace OpenSkyhawk;

static const StepperConfig MOTOR_CFG = {
    720, StepPattern::SWITEC_6STATE, kSwitecDefaultAccel, kSwitecDefaultAccelN,
    HomeMode::STALL, false, { false, 0, 0 }, 0, 0, 0, 720, false, 0, false, 0,
};
StepperMotor gMotor(PinRef(), PinRef(), PinRef(), PinRef(), MOTOR_CFG);

// Non-linear: half the value range covers only 1/6 of the travel, then it opens up.
static const uint16_t CURVE_IN[3]  = {     0, 32768, 65535 };
static const uint16_t CURVE_OUT[3] = {     0,   100,   600 };
static const GaugeCal CAL = { 0, 0, false, CURVE_IN, CURVE_OUT, 3 };

NeedleGauge gGauge(0x2000, 0xFFFF, gMotor, CAL);

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== NeedleGauge calibration ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    check("clamp low: 0 -> 0",        gGauge.debugValueToPos(0)     == 0);
    check("breakpoint: 32768 -> 100", gGauge.debugValueToPos(32768) == 100);
    check("clamp high: 65535 -> 600", gGauge.debugValueToPos(65535) == 600);
    check("interp lo seg: 16384 -> 50",  gGauge.debugValueToPos(16384) == 50);
    check("interp hi seg: 49152 -> 350", gGauge.debugValueToPos(49152) == 350);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
