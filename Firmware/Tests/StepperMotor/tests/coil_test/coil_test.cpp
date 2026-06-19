// StepperMotor — static coil-energize diagnostic (bench)
//
// Bypasses the whole StepperMotor stack (and all libraries): raw digitalWrite on the 4 coil
// pins, holding each phase 1.5 s. Use to isolate "is the motor wired + powered" from stepping.
//   - Multimeter each pin to GND: a driven-HIGH pin reads ~3.3 V, LOW ~0 V (and it changes every 1.5 s).
//   - The needle should JUMP/hold to a detent each phase. If it jumps, motor + wiring + GPIO are
//     all good and the issue is the step sequence/speed. If nothing moves, it's power, an open
//     coil, or the motor.
//
// Coils: A = PA0/PA1, B = PA4/PA5.

#include <Arduino.h>

static void phase(bool a0, bool a1, bool b2, bool b3) {
    digitalWrite(PA0, a0); digitalWrite(PA1, a1);
    digitalWrite(PA4, b2); digitalWrite(PA5, b3);
}

// PC13 = Blue Pill onboard LED (active-LOW). Blinking it proves the firmware is actually running.
static bool led = false;
static void step(bool a0, bool a1, bool b2, bool b3) {
    led = !led;
    digitalWrite(PC13, led ? LOW : HIGH);   // toggle onboard LED each phase
    phase(a0, a1, b2, b3);
    delay(1500);
}

void setup() {
    pinMode(PA0, OUTPUT); pinMode(PA1, OUTPUT);
    pinMode(PA4, OUTPUT); pinMode(PA5, OUTPUT);
    pinMode(PC13, OUTPUT);                   // onboard LED — "is the firmware running" indicator
}

void loop() {
    step(1, 0, 0, 0);   // coil A +
    step(0, 1, 0, 0);   // coil A -
    step(0, 0, 1, 0);   // coil B +
    step(0, 0, 0, 1);   // coil B -
    step(1, 0, 1, 0);   // A+ B+ (needle holds at a diagonal)
    step(0, 1, 0, 1);   // A- B-
}
