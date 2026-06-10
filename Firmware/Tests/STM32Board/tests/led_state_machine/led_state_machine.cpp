// STM32Board — LED state machine test
//
// Cycles through all 6 LedStates every 4 seconds.
// Observe PB14 (red) and PB15 (green) against the table below.
//
// Step 0 — OFF      : both LEDs off         (2 s before begin())
// Step 1 — BOOTING  : red  slow blink 1 Hz  (CanStatus::STARTING)
// Step 2 — NORMAL   : green slow blink 1 Hz (CanStatus::NORMAL)
// Step 3 — CAN_ERROR: red  fast blink 4 Hz  (CanStatus::TX_ERROR)
// Step 4 — BUS_OFF  : red  solid on         (CanStatus::BUS_OFF)
// Step 5 — WARNING  : red/green alternating 2 Hz (setWarning())
//
// After all six, loops back to step 1 (OFF only shown once at boot).

#include <STM32Board.h>
#include <CANProtocol.h>

static constexpr uint32_t kDwellMs = 4000;
static uint8_t  _step   = 0;
static uint32_t _lastMs = 0;

static void applyStep(uint8_t step) {
    auto& d = STM32Board::diagSerial();
    switch (step) {
        case 0:
            d.println(F("[0] OFF      : both LEDs off"));
            break;
        case 1:
            STM32Board::onCanStatus(CanStatus::STARTING);
            d.println(F("[1] BOOTING  : red slow blink 1 Hz"));
            break;
        case 2:
            STM32Board::onCanStatus(CanStatus::NORMAL);
            d.println(F("[2] NORMAL   : green slow blink 1 Hz"));
            break;
        case 3:
            STM32Board::onCanStatus(CanStatus::TX_ERROR);
            d.println(F("[3] CAN_ERROR: red fast blink 4 Hz"));
            break;
        case 4:
            STM32Board::onCanStatus(CanStatus::BUS_OFF);
            d.println(F("[4] BUS_OFF  : red solid on"));
            break;
        case 5:
            STM32Board::setWarning();
            d.println(F("[5] WARNING  : red/green alternating 2 Hz"));
            break;
    }
}

void setup() {
    // Step 0: OFF — both pins low before begin() configures them as outputs
    delay(kDwellMs);

    STM32Board::setDebug(true);
    STM32Board::begin();  // enters BOOTING immediately
    STM32Board::log("=== STM32Board LED state machine test ===");
    STM32Board::log("Observe PB14 (red) and PB15 (green). 4 s per state.");

    _step   = 1;  // begin() already set BOOTING; announce it
    _lastMs = millis();
    applyStep(_step);
}

void loop() {
    STM32Board::tick();

    uint32_t now = millis();
    if (now - _lastMs >= kDwellMs) {
        _lastMs = now;
        _step = (_step % 5) + 1;  // cycle steps 1–5
        applyStep(_step);
    }
}
