// STM32Board — DiagSerial gate test
//
// Verifies that log() produces no output when debug is off, and emits the
// expected string when debug is on.
//
// Open a serial monitor at 115200 baud on the USART1 tap (PA9 TX).
//
// Expected output (one cycle every 3 seconds):
//   [OFF ] nothing should appear here
//   [ON  ] debug is ON — this line should appear
//   [ON  ] log() is gated correctly
//
// If you see output during the [OFF] phase, the gate is broken.

#include <STM32Board.h>
#include <CANProtocol.h>

static constexpr uint32_t kPhaseMs = 3000;
static uint32_t _lastMs = 0;
static uint8_t  _phase  = 0;  // 0 = debug off, 1 = debug on

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::log("=== STM32Board DiagSerial gate test ===");
    STM32Board::log("Open serial monitor at 115200 on PA9 TX.");
}

void loop() {
    STM32Board::tick();

    uint32_t now = millis();
    if (now - _lastMs < kPhaseMs) return;
    _lastMs = now;

    if (_phase == 0) {
        STM32Board::setDebug(false);
        // These calls must produce NO output:
        STM32Board::log("[OFF] nothing should appear here");
        STM32Board::log("[OFF] gate is broken if you see this");
        // Re-enable to announce the transition:
        STM32Board::setDebug(true);
        STM32Board::diagSerial().println(F("[GATE] debug was OFF — lines above suppressed"));
    } else {
        STM32Board::setDebug(true);
        STM32Board::log("[ON ] debug is ON — this line should appear");
        STM32Board::log("[ON ] log() is gated correctly");
    }

    _phase = (_phase + 1) % 2;
}
