// APN-153 DRIFT gauge motion simulation (bench) — REAL NeedleGauge + StepperMotor path, fed
// synthetic DCS drift values. Center-zero gauge: value 32768 = centre, 0 / 65535 = full
// left / right drift. Mapped onto the bench BKA-30's measured ~960-step travel; drift arc
// ±150 steps (±50°) about the 480-step mechanical centre.
//
// v2: all moves are CONTINUOUS slews (no stop-start) + a FASTER accel table — full-step air-core
// looks jerky creeping slowly (1/3°/step, no microstepping) but smooths out at speed. Each feed()
// is a single smooth slew to the new drift value. Watch: smooth throughout, ends reached clean,
// centre holds across cycles. If it loses steps at this rate, that's the top-speed limit.

#include <Arduino.h>
#include <STM32Board.h>
#include <Drivers/StepperMotor/StepperMotor.h>
#include <Outputs/NeedleGauge/NeedleGauge.h>
#include <A4EC_OutputIds.h>

using namespace OpenSkyhawk;

// Datasheet-limit envelope: top 555µs ≈ 1800 steps/s ≈ 600°/s — the BKA-30 spec max-with-accel.
static const AccelPoint FAST_ACCEL[5] = { {20,3000},{50,1500},{100,1000},{150,800},{300,555} };

static const StepperConfig DRIFT_MOTOR = {
    /* stepsPerRev */ 1060,                   // STALL home drive = ~960 range + 100 margin to seat
    /* pattern     */ StepPattern::SWITEC_6STATE,
    /* accel       */ FAST_ACCEL,
    /* accelN      */ 5,
    /* home        */ HomeMode::STALL,
    /* homeSeekCW  */ false,
    /* sensor      */ { false, 0, 0 },
    /* homePos     */ 0,                       // right stop = 0
    /* parkPos     */ 480,                     // mechanical centre of the ~960 travel
    /* minPos      */ 0,
    /* maxPos      */ 960,
    /* wrap        */ false,
    /* deadband    */ 1,
    /* autoRecal   */ false,
    /* recalMs     */ 0,
};
StepperMotor driftMotor(PinRef(PA0), PinRef(PA1), PinRef(PA4), PinRef(PA5), DRIFT_MOTOR);

// Centre-zero arc: value 0 → 330, 32768 → 480 (centre), 65535 → 630. ±150 steps about centre.
static const GaugeCal DRIFT_CAL = { 330, 630, false, nullptr, nullptr, 0 };
NeedleGauge drift(A_4E_C_APN153_DRIFT_GAUGE, A_4E_C_APN153_DRIFT_GAUGE_AM, driftMotor, DRIFT_CAL);

static void feed(uint16_t value) { drift.onControlPacket(A_4E_C_APN153_DRIFT_GAUGE, value); }

// Slew smoothly to the last fed value — run the engine continuously (no mid-move pause).
static void slew() {
    uint32_t t = millis();
    while (!driftMotor.debugStopped() && millis() - t < 4000) drift.update();
}

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== APN-153 DRIFT gauge sim v2 (fast, smooth) ===");
    drift.configure();                         // homes (seat stop), then parks at centre (480)
    STM32Board::diagSerial().println(driftMotor.homed() ? "homed + centred" : "HOME FAILED");
}

void loop() {
    auto& s = STM32Board::diagSerial();
    s.println("smooth drift sweep");

    feed(45000); slew(); delay(250);           // right of centre
    feed(20000); slew(); delay(250);           // left of centre
    feed(38000); slew(); delay(250);           // small right
    feed(27000); slew(); delay(250);           // small left
    feed(65535); slew(); delay(250);           // full right drift
    feed(0);     slew(); delay(250);           // full left drift
    feed(32768); slew(); delay(900);           // recentre, hold

    s.println("cycle done");
}
