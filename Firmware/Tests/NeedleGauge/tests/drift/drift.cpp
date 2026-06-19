// NeedleGauge — APN-153 DRIFT needle (tier 1: end-to-end decode → motor target)
//
// Centre-zero gauge: DCS 0 → full-left, 32768 → centre (0), 65535 → full-right. Drives the
// full path (controlId filter → mask → valueToPos → motor.moveTo) and reads the motor target.

#include <Arduino.h>
#include <STM32Board.h>
#include <Drivers/StepperMotor/StepperMotor.h>
#include <Outputs/NeedleGauge/NeedleGauge.h>
#include <A4EC_OutputIds.h>

using namespace OpenSkyhawk;

// Motor with a ±200-step travel so the ±150 drift arc passes unclamped; deadband 0.
static const StepperConfig MOTOR_CFG = {
    720, StepPattern::SWITEC_6STATE, kSwitecDefaultAccel, kSwitecDefaultAccelN,
    HomeMode::STALL, false, { false, 0, 0 }, 0, 0, -200, 200, false, 0, false, 0,
};
StepperMotor gMotor(PinRef(), PinRef(), PinRef(), PinRef(), MOTOR_CFG);

// Drift arc ±150 steps about centre.
static const GaugeCal DRIFT_CAL = { -150, 150, false, nullptr, nullptr, 0 };

NeedleGauge gDrift(A_4E_C_APN153_DRIFT_GAUGE, A_4E_C_APN153_DRIFT_GAUGE_AM, gMotor, DRIFT_CAL);

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== NeedleGauge drift (APN-153) ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    // mapping endpoints + centre
    check("value 0     -> -150 (full left)",  gDrift.debugValueToPos(0)     == -150);
    check("value 32768 -> 0 (centre)",        gDrift.debugValueToPos(32768) == 0);
    check("value 65535 -> 150 (full right)",  gDrift.debugValueToPos(65535) == 150);

    // end-to-end: packet → motor target
    gDrift.onControlPacket(A_4E_C_APN153_DRIFT_GAUGE, 32768);
    check("packet centre -> motor target 0",  gMotor.debugTargetStep() == 0);

    gDrift.onControlPacket(A_4E_C_APN153_DRIFT_GAUGE, 0);
    check("packet 0 -> motor target -150",    gMotor.debugTargetStep() == -150);

    gDrift.onControlPacket(A_4E_C_APN153_DRIFT_GAUGE, 65535);
    check("packet 65535 -> motor target 150", gMotor.debugTargetStep() == 150);

    // wrong controlId is ignored (target stays at 150)
    gDrift.onControlPacket(0x9999, 0);
    check("other controlId ignored",          gMotor.debugTargetStep() == 150);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
