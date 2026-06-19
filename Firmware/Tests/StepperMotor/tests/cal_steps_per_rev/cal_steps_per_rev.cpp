// StepperMotor — steps-per-revolution calibration (bench, AHN method)
//
// Inches the motor forward past a zero sensor twice and reports the step count between
// crossings = the motor's true steps/rev. Seed that into a panel's StepperConfig.stepsPerRev
// and the degree→step conversions.
//
// Hardware: X27 coils on PA0/PA1/PA4/PA5; a zero sensor (opto/hall/switch, active-LOW)
// on PA6 with a pull-up. The motor drives forward; watch the serial count.

#include <Arduino.h>
#include <STM32Board.h>
#include <Drivers/StepperMotor/StepperMotor.h>

using namespace OpenSkyhawk;

static constexpr uint8_t SENSE_PIN = PA6;

static const StepperConfig CFG = {
    /* stepsPerRev       */ 720,                    // nominal; this test measures the real value
    /* pattern           */ StepPattern::SWITEC_6STATE,
    /* accel             */ kSwitecDefaultAccel,
    /* accelN            */ kSwitecDefaultAccelN,
    /* home              */ HomeMode::STALL,
    /* homeSeekClockwise */ false,
    /* sensor            */ { false, 0, 0 },
    /* homePosition      */ 0,
    /* parkPosition      */ 0,
    /* minPos            */ 0,
    /* maxPos            */ 4000,
    /* wrap              */ false,
    /* deadband          */ 0,
    /* autoRecal         */ false,
    /* recalDebounceMs   */ 0,
};

StepperMotor gMotor(PinRef(PA0), PinRef(PA1), PinRef(PA4), PinRef(PA5), CFG);

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== StepperMotor cal_steps_per_rev (bench) ===");

    pinMode(SENSE_PIN, INPUT_PULLUP);
    gMotor.configure();

    int crossings = 0;
    int32_t firstPos = 0;
    bool last = digitalRead(SENSE_PIN);   // HIGH = idle
    gMotor.moveTo(3500);                   // drive forward several revolutions

    while (crossings < 2 && !gMotor.debugStopped()) {
        gMotor.update();
        bool now = digitalRead(SENSE_PIN);
        if (last == HIGH && now == LOW) {  // falling edge = zero mark
            crossings++;
            if (crossings == 1) firstPos = gMotor.position();
            else {
                int32_t perRev = gMotor.position() - firstPos;
                STM32Board::diagSerial().print("Measured steps/rev: ");
                STM32Board::diagSerial().println(perRev);
            }
            delay(5);                       // debounce
        }
        last = now;
    }
    if (crossings < 2) STM32Board::diagSerial().println("Did not see two crossings — check sensor wiring");
}

void loop() {}
