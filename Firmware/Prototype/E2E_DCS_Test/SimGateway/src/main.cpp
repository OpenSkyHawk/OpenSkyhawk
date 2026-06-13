// E2E_DCS_Test — SimGateway (RP2040)
// No HIDAxis/HIDButton — DCS-BIOS only test.
// LED_BUILTIN blinks to show connection state (USB CDC shared with DCS-BIOS relay).
//   500 ms blink: idle — DCS not yet connected
//   100 ms blink: active — DCS CDC bytes seen

#include <SimGateway.h>
#include <HIDControls.h>

static uint32_t _ledToggleMs = 0;
static bool     _ledState    = false;
static bool     _dcsActive   = false;

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    SimGateway::setup(Serial1);
}

void loop() {
    // Check BEFORE SimGateway::loop() drains the buffer.
    bool sawCdc = Serial.available() > 0;
    SimGateway::loop();
    if (sawCdc) _dcsActive = true;

    uint32_t interval = _dcsActive ? 100 : 500;
    uint32_t now = millis();
    if (now - _ledToggleMs >= interval) {
        _ledToggleMs = now;
        _ledState = !_ledState;
        digitalWrite(LED_BUILTIN, _ledState);
    }
}
