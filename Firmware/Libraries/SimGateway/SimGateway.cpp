#ifdef ARDUINO_ARCH_RP2040

#include "SimGateway.h"
#include <USB.h>
#include <MGS_Pico_Joystick.h>

// ── HIDAxis ───────────────────────────────────────────────────────────────────

namespace OpenSkyhawk {

HIDAxis* HIDAxis::_head = nullptr;

HIDAxis::HIDAxis(uint16_t controlId, uint8_t axisIndex)
    : _next(nullptr), _controlId(controlId), _axisIndex(axisIndex)
{
    _next = _head;
    _head = this;
}

HIDAxis* HIDAxis::head()                   { return _head; }
uint16_t HIDAxis::controlId() const        { return _controlId; }
HIDAxis* HIDAxis::next() const             { return _next; }
void     HIDAxis::dispatch(uint16_t value) {
    Joystick.SetAxis(_axisIndex, (int16_t)(value - 32768));
}

// ── HIDButton ─────────────────────────────────────────────────────────────────

HIDButton* HIDButton::_head = nullptr;

HIDButton::HIDButton(uint16_t controlId, uint8_t buttonIndex)
    : _next(nullptr), _controlId(controlId), _buttonIndex(buttonIndex)
{
    _next = _head;
    _head = this;
}

HIDButton* HIDButton::head()                   { return _head; }
uint16_t   HIDButton::controlId() const        { return _controlId; }
HIDButton* HIDButton::next() const             { return _next; }
void       HIDButton::dispatch(uint16_t value) {
    Joystick.SetButton(_buttonIndex, value != 0);
}

} // namespace OpenSkyhawk

// ── Internal parser ───────────────────────────────────────────────────────────

namespace {

enum class ParserState : uint8_t { IDLE, GOT_AA, IN_FRAME };

HardwareSerial* _uart     = nullptr;
ParserState     _state    = ParserState::IDLE;
uint8_t         _frameBuf[4];
uint8_t         _framePos = 0;

#ifdef SIMGATEWAY_TEST
// Captured by test files; updated on each HID dispatch.
uint16_t _sgtest_lastControlId = 0;
uint16_t _sgtest_lastValue     = 0;
uint8_t  _sgtest_dispatchCount = 0;
#endif

// Process one UART byte through the state machine.
// Returns true if a HID setter fired (Joystick.Set* was called).
bool _processByte(uint8_t b) {
    switch (_state) {

        case ParserState::IDLE:
            if (b == 0xAA) {
                _state = ParserState::GOT_AA;
            } else {
                Serial.write(b); // DCS-BIOS byte → CDC
            }
            return false;

        case ParserState::GOT_AA:
            if (b == 0x55) {
                _framePos = 0;
                _state    = ParserState::IN_FRAME;
            } else {
                // Resync: 0xAA was not a magic prefix — forward both bytes
                Serial.write(0xAA);
                Serial.write(b);
                _state = ParserState::IDLE;
            }
            return false;

        case ParserState::IN_FRAME: {
            _frameBuf[_framePos++] = b;
            if (_framePos < 4) return false;

            uint16_t controlId = (uint16_t)_frameBuf[0] | ((uint16_t)_frameBuf[1] << 8);
            uint16_t value     = (uint16_t)_frameBuf[2] | ((uint16_t)_frameBuf[3] << 8);
            bool fired = false;

            for (auto* a = OpenSkyhawk::HIDAxis::head(); a; a = a->next()) {
                if (a->controlId() == controlId) { a->dispatch(value); fired = true; }
            }
            for (auto* btn = OpenSkyhawk::HIDButton::head(); btn; btn = btn->next()) {
                if (btn->controlId() == controlId) { btn->dispatch(value); fired = true; }
            }

#ifdef SIMGATEWAY_TEST
            if (fired) {
                _sgtest_lastControlId = controlId;
                _sgtest_lastValue     = value;
                _sgtest_dispatchCount++;
            }
#endif

            _state = ParserState::IDLE;
            return fired;
        }
    }
    return false;
}

} // anonymous namespace

// ── SimGateway ────────────────────────────────────────────────────────────────

namespace SimGateway {

void setup(HardwareSerial& uart) {
    _uart = &uart;

    // USB identity must be set before TinyUSB enumerates
    USB.setManufacturer("OpenSkyhawk");
    USB.setProduct("A-4E Skyhawk");
    USB.setVIDPID(0x2E8A, 0x4134);

    _uart->begin(250000);
    Joystick.begin(); // USB identity must be set above before this call
}

void loop() {
    // 1. Forward CDC → UART (PC DCS-BIOS stream to PanelBridge)
    while (Serial.available()) {
        _uart->write(Serial.read());
    }

    // 2. Drain UART; HID frames dispatched, DCS-BIOS bytes forwarded to CDC
    bool anyFired = false;
    while (_uart->available()) {
        anyFired |= _processByte(_uart->read());
    }

    // 3. Flush one HID report if any setter fired this iteration
    if (anyFired) Joystick.Send();
}

#ifdef SIMGATEWAY_TEST
bool feedByte(uint8_t b)  { return _processByte(b); }
void resetParser()         { _state = ParserState::IDLE; _framePos = 0; }
#endif

} // namespace SimGateway

#endif // ARDUINO_ARCH_RP2040
