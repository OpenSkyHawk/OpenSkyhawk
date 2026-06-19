// StepperMotor — motion-profile assertions (tier 1: deterministic, no motor)
//
// Drives the accel engine directly via debugAdvance() (no micros() gate, no hardware)
// and inspects the step-interval sequence to prove the ported SwitecX25 trapezoid:
//   - starts slow (first interval = slowest table delay)
//   - reaches top speed on a long move (min interval = last table delay = 600 µs)
//   - decelerates to a stop EXACTLY at target (no residual offset)
//   - a short move stays triangular (never reaches cruise)
//   - a reversal settles cleanly at the new target
//
// Coils are NC PinRefs — nothing is driven; only the position/velocity math runs.

#include <Arduino.h>
#include <STM32Board.h>
#include <Drivers/StepperMotor/StepperMotor.h>

using namespace OpenSkyhawk;

static const StepperConfig CFG = {
    /* stepsPerRev       */ 720,
    /* pattern           */ StepPattern::SWITEC_6STATE,
    /* accel             */ kSwitecDefaultAccel,
    /* accelN            */ kSwitecDefaultAccelN,
    /* home              */ HomeMode::STALL,
    /* homeSeekClockwise */ false,
    /* sensor            */ { false, 0, 0 },
    /* homePosition      */ 0,
    /* parkPosition      */ 0,
    /* minPos            */ 0,
    /* maxPos            */ 720,
    /* wrap              */ false,
    /* deadband          */ 0,
    /* autoRecal         */ false,
    /* recalDebounceMs   */ 0,
};

StepperMotor gMotor(PinRef(), PinRef(), PinRef(), PinRef(), CFG);

// Run the engine to a standstill, capturing min/first interval and the peak velocity.
static void runMove(int32_t target, uint16_t& firstDelay, uint16_t& minDelay,
                    uint16_t& maxVel, uint32_t& steps) {
    gMotor.moveTo(target);
    firstDelay = 0; minDelay = 0xFFFF; maxVel = 0; steps = 0;
    for (uint32_t i = 0; i < 5000 && !gMotor.debugStopped(); i++) {
        gMotor.debugAdvance();
        uint16_t d = gMotor.debugMicroDelay();
        if (steps == 0) firstDelay = d;
        if (d < minDelay) minDelay = d;
        if (gMotor.debugVel() > maxVel) maxVel = gMotor.debugVel();
        steps++;
    }
}

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== StepperMotor motion_profile ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    gMotor.configure();

    // ── Long move: should accelerate to top speed, then decelerate to target ──────
    uint16_t firstDelay, minDelay, maxVel; uint32_t steps;
    runMove(700, firstDelay, minDelay, maxVel, steps);
    check("long: settles exactly at target", gMotor.debugCurrentStep() == 700 && gMotor.debugStopped());
    check("long: starts at slowest delay (3000us)", firstDelay == 3000);
    check("long: reaches top speed (minDelay 600us)", minDelay == 600);
    check("long: peak vel hits maxVel (300)", maxVel == 300);

    // ── Short move: triangular — never reaches cruise speed ───────────────────────
    uint16_t f2, min2, mv2; uint32_t s2;
    runMove(710, f2, min2, mv2, s2);          // +10 steps from 700
    check("short: settles exactly at target", gMotor.debugCurrentStep() == 710 && gMotor.debugStopped());
    check("short: stays slow (minDelay > 600us)", min2 > 600);

    // ── Reversal: long move back to zero settles cleanly ──────────────────────────
    uint16_t f3, min3, mv3; uint32_t s3;
    runMove(0, f3, min3, mv3, s3);
    check("reverse: settles exactly at 0", gMotor.debugCurrentStep() == 0 && gMotor.debugStopped());
    check("reverse: reaches top speed again", min3 == 600);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
