// STM32Board — LED state machine test
//
// Cycles through each CanStatus value every 4 seconds and prints the expected
// LED behaviour. Observe PB14 (red) and PB15 (green) against the table below.
//
// Expected patterns:
//   STARTING  → BOOTING   : red  slow blink 1 Hz (500 ms on/off)
//   NORMAL    → NORMAL    : green slow blink 1 Hz (500 ms on/off)
//   TX_ERROR  → CAN_ERROR : red  fast blink 4 Hz (125 ms on/off)
//   BUS_OFF   → BUS_OFF   : red  solid on, green off
//
// After all four states, loops back to STARTING.

#include <STM32Board.h>
#include <CANProtocol.h>

static const struct {
    CanStatus status;
    const char* label;
    const char* expected;
} kStates[] = {
    { CanStatus::STARTING,  "STARTING",  "red  slow blink 1 Hz"           },
    { CanStatus::NORMAL,    "NORMAL",    "green slow blink 1 Hz"           },
    { CanStatus::TX_ERROR,  "TX_ERROR",  "red  fast blink 4 Hz"           },
    { CanStatus::BUS_OFF,   "BUS_OFF",   "red  solid on, green off"       },
};
static constexpr uint8_t kNumStates  = sizeof(kStates) / sizeof(kStates[0]);
static constexpr uint32_t kDwellMs   = 4000;

static uint8_t  _idx   = 0;
static uint32_t _lastMs = 0;

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::log("=== STM32Board LED state machine test ===");
    STM32Board::log("Observe PB14 (red) and PB15 (green).");
}

void loop() {
    STM32Board::tick();

    uint32_t now = millis();
    if (now - _lastMs >= kDwellMs) {
        _lastMs = now;
        auto& s = kStates[_idx];
        STM32Board::onCanStatus(s.status);

        auto& d = STM32Board::diagSerial();
        d.print(F("[STATE] "));
        d.print(s.label);
        d.print(F(" -> "));
        d.println(s.expected);

        _idx = (_idx + 1) % kNumStates;
    }
}
