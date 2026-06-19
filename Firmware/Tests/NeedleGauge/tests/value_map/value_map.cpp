// NeedleGauge — linear value→position mapping (tier 1: deterministic)
//
// 0 → minTravel, 65535 → maxTravel, linear between. reverse flips the input.
// Mapping is checked directly via debugValueToPos(); no motor motion needed.

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

static const GaugeCal CAL_FWD = { 0, 600, false, nullptr, nullptr, 0 };
static const GaugeCal CAL_REV = { 0, 600, true,  nullptr, nullptr, 0 };

NeedleGauge gFwd(0x1000, 0xFFFF, gMotor, CAL_FWD);
NeedleGauge gRev(0x1001, 0xFFFF, gMotor, CAL_REV);

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== NeedleGauge value_map ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    check("fwd 0 -> 0",          gFwd.debugValueToPos(0)     == 0);
    check("fwd 16384 -> 150",    gFwd.debugValueToPos(16384) == 150);
    check("fwd 32768 -> 300",    gFwd.debugValueToPos(32768) == 300);
    check("fwd 65535 -> 600",    gFwd.debugValueToPos(65535) == 600);

    check("rev 0 -> 600",        gRev.debugValueToPos(0)     == 600);
    check("rev 65535 -> 0",      gRev.debugValueToPos(65535) == 0);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
