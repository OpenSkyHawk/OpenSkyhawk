// STM32Board — CanStatus → LedState wiring test
//
// Calls onCanStatus() with each CanStatus value and prints the expected LED
// behaviour. Observe PB14 (red) and PB15 (green) against the serial output.
//
// This test does NOT require a CAN bus or second node — onCanStatus() is called
// directly without any real CAN activity.
//
// Expected mapping:
//   CanStatus::STARTING  → BOOTING   : red  slow blink 1 Hz
//   CanStatus::NORMAL    → NORMAL    : green slow blink 1 Hz
//   CanStatus::TX_ERROR  → CAN_ERROR : red  fast blink 4 Hz
//   CanStatus::BUS_OFF   → BUS_OFF   : red  solid on, green off
//
// After all four, loops back. Dwell time: 5 seconds per state.

#include <STM32Board.h>
#include <CANProtocol.h>

static constexpr uint32_t kDwellMs = 5000;
static uint32_t _lastMs = 0;
static uint8_t  _step   = 0;

struct TestStep {
    CanStatus status;
    const char* statusName;
    const char* ledState;
    const char* ledBehaviour;
};

static const TestStep kSteps[] = {
    { CanStatus::STARTING, "STARTING", "BOOTING",   "red  slow blink 1 Hz (500 ms)" },
    { CanStatus::NORMAL,   "NORMAL",   "NORMAL",    "green slow blink 1 Hz (500 ms)" },
    { CanStatus::TX_ERROR, "TX_ERROR", "CAN_ERROR", "red  fast blink 4 Hz (125 ms)" },
    { CanStatus::BUS_OFF,  "BUS_OFF",  "BUS_OFF",   "red  solid on, green off"       },
};
static constexpr uint8_t kNumSteps = sizeof(kSteps) / sizeof(kSteps[0]);

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::log("=== STM32Board CanStatus wiring test ===");
    STM32Board::log("Observe PB14 (red) and PB15 (green).");
    STM32Board::log("5 seconds per state — match LED to expected pattern.");
}

void loop() {
    STM32Board::tick();

    uint32_t now = millis();
    if (now - _lastMs < kDwellMs) return;
    _lastMs = now;

    auto& step = kSteps[_step];
    STM32Board::onCanStatus(step.status);

    auto& d = STM32Board::diagSerial();
    d.print(F("[STEP "));
    d.print(_step + 1);
    d.print(F("/4] CanStatus::"));
    d.print(step.statusName);
    d.print(F(" -> LedState::"));
    d.print(step.ledState);
    d.print(F(" : "));
    d.println(step.ledBehaviour);

    _step = (_step + 1) % kNumSteps;
}
