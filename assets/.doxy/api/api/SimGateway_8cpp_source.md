

# File SimGateway.cpp

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**SimGateway**](dir_a54aa0246e1c520ae49dfef506a428ca.md) **>** [**SimGateway.cpp**](SimGateway_8cpp.md)

[Go to the documentation of this file](SimGateway_8cpp.md)


```C++
#ifdef ARDUINO_ARCH_RP2040

#include "SimGateway.h"

// ── TinyUSB HID backend (production builds only) ──────────────────────────────
//
// SIMGATEWAY_TEST builds substitute no-op stubs below. The #ifndef guard prevents
// Adafruit_TinyUSB.h from being included in test builds, keeping tests free of USB
// enumeration side effects.

#ifndef SIMGATEWAY_TEST
#include <Adafruit_TinyUSB.h>

namespace {

// HID report: 128 buttons (16 bytes) + 4 hat switches (4-bit each = 2 bytes) + 8 axes (16 bytes)
struct __attribute__((packed)) HIDReport {
    uint8_t buttons[16]; // 128 × 1-bit buttons (button 0 = bit 0 of byte 0)
    uint8_t hats[2];     // 4 × 4-bit hat values; nibble value ≥ 8 = null / centered
    int16_t axes[8];     // X, Y, Z, Rx, Ry, Rz, Slider, Dial
};

static const uint8_t desc_hid_report[] = {
    // Joystick application collection
    0x05, 0x01,        // Usage Page (Generic Desktop)
    0x09, 0x04,        // Usage (Joystick)
    0xA1, 0x01,        // Collection (Application)

    // 128 Buttons (1-bit each, 16 bytes total)
    0x05, 0x09,        //   Usage Page (Button)
    0x19, 0x01,        //   Usage Minimum (1)
    0x29, 0x80,        //   Usage Maximum (128)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x80,        //   Report Count (128)
    0x81, 0x02,        //   Input (Data, Variable, Absolute)

    // 4 Hat switches (4-bit each, 2 bytes total)
    // Logical 0-7 = N/NE/E/SE/S/SW/W/NW; value ≥ 8 = null (centered).
    0x05, 0x01,        //   Usage Page (Generic Desktop)
    0x09, 0x39,        //   Usage (Hat Switch) — hat 0
    0x09, 0x39,        //   Usage (Hat Switch) — hat 1
    0x09, 0x39,        //   Usage (Hat Switch) — hat 2
    0x09, 0x39,        //   Usage (Hat Switch) — hat 3
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x07,        //   Logical Maximum (7)
    0x35, 0x00,        //   Physical Minimum (0 degrees)
    0x46, 0x3B, 0x01,  //   Physical Maximum (315 degrees)
    0x65, 0x14,        //   Unit (Degrees)
    0x75, 0x04,        //   Report Size (4)
    0x95, 0x04,        //   Report Count (4)
    0x81, 0x42,        //   Input (Data, Variable, Absolute, Null State)

    // 8 Axes: X, Y, Z, Rx, Ry, Rz, Slider, Dial (16-bit signed each, 16 bytes total)
    0x05, 0x01,        //   Usage Page (Generic Desktop)
    0x09, 0x30,        //   Usage (X)
    0x09, 0x31,        //   Usage (Y)
    0x09, 0x32,        //   Usage (Z)
    0x09, 0x33,        //   Usage (Rx)
    0x09, 0x34,        //   Usage (Ry)
    0x09, 0x35,        //   Usage (Rz)
    0x09, 0x36,        //   Usage (Slider)
    0x09, 0x37,        //   Usage (Dial)
    0x16, 0x00, 0x80,  //   Logical Minimum (-32768)
    0x26, 0xFF, 0x7F,  //   Logical Maximum (32767)
    0x75, 0x10,        //   Report Size (16)
    0x95, 0x08,        //   Report Count (8)
    0x81, 0x02,        //   Input (Data, Variable, Absolute)

    0xC0               // End Collection
};

static HIDReport _hidReport = {};
static Adafruit_USBD_HID _usbHid(desc_hid_report, sizeof(desc_hid_report),
                                  HID_ITF_PROTOCOL_NONE, 2, false);

static void _hidBegin() {
    // All hat nibbles start centered (null state = 0xF per nibble).
    _hidReport.hats[0] = 0xFF;
    _hidReport.hats[1] = 0xFF;
    _usbHid.begin();
    // Block until the host enumerates (2 s timeout: handles benchtop use without USB host).
    uint32_t t = millis();
    while (!TinyUSBDevice.mounted() && (millis() - t) < 2000) delay(1);
}

static void _hidSetAxis(uint8_t axisIndex, int16_t value) {
    if (axisIndex < 8) _hidReport.axes[axisIndex] = value;
}

static void _hidSetButton(uint8_t buttonIndex, bool pressed) {
    if (buttonIndex >= 128) return;
    uint8_t byte_idx = buttonIndex / 8;
    uint8_t bit_mask = 1u << (buttonIndex % 8);
    if (pressed) _hidReport.buttons[byte_idx] |=  bit_mask;
    else         _hidReport.buttons[byte_idx] &= ~bit_mask;
}

static void _hidSetHat(uint8_t hatIndex, uint8_t direction) {
    if (hatIndex >= 4) return;
    // direction: 0=center→0xF, 1=N→0, 2=NE→1, …, 8=NW→7; >8→0xF (center)
    uint8_t hid_val    = (direction == 0 || direction > 8) ? 0xF : (direction - 1);
    uint8_t byte_idx   = hatIndex / 2;
    uint8_t nibble_idx = hatIndex % 2;
    if (nibble_idx == 0) _hidReport.hats[byte_idx] = (_hidReport.hats[byte_idx] & 0xF0) | (hid_val & 0x0F);
    else                 _hidReport.hats[byte_idx] = (_hidReport.hats[byte_idx] & 0x0F) | ((hid_val & 0x0F) << 4);
}

static void _hidSend() {
    if (_usbHid.ready()) _usbHid.sendReport(0, &_hidReport, sizeof(_hidReport));
}

} // anonymous namespace

#else // SIMGATEWAY_TEST — no-op HID stubs

namespace {
static void _hidBegin()                              {}
static void _hidSetAxis(uint8_t, int16_t)            {}
static void _hidSetButton(uint8_t, bool)             {}
static void _hidSetHat(uint8_t, uint8_t)             {}
static void _hidSend()                               {}
} // anonymous namespace

#endif // SIMGATEWAY_TEST

// ── Test capture globals ───────────────────────────────────────────────────────

#ifdef SIMGATEWAY_TEST
uint16_t _sgtest_lastControlId = 0;
uint16_t _sgtest_lastValue     = 0;
uint8_t  _sgtest_dispatchCount = 0;
#endif

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
    _hidSetAxis(_axisIndex, (int16_t)(value - 32768));
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
    _hidSetButton(_buttonIndex, value != 0);
}

// ── HIDHatSwitch ──────────────────────────────────────────────────────────────

HIDHatSwitch* HIDHatSwitch::_head = nullptr;

HIDHatSwitch::HIDHatSwitch(uint16_t controlId, uint8_t hatIndex)
    : _next(nullptr), _controlId(controlId), _hatIndex(hatIndex)
{
    _next = _head;
    _head = this;
}

HIDHatSwitch* HIDHatSwitch::head()                   { return _head; }
uint16_t      HIDHatSwitch::controlId() const        { return _controlId; }
HIDHatSwitch* HIDHatSwitch::next() const             { return _next; }
void          HIDHatSwitch::dispatch(uint16_t value) {
    _hidSetHat(_hatIndex, (uint8_t)(value > 8 ? 0 : value));
}

} // namespace OpenSkyhawk

// ── Internal parser ───────────────────────────────────────────────────────────

namespace {

enum class ParserState : uint8_t { IDLE, GOT_AA, IN_FRAME };

SerialUART*     _uart     = nullptr;
ParserState     _state    = ParserState::IDLE;
uint8_t         _frameBuf[4];
uint8_t         _framePos = 0;

#ifdef SIMGATEWAY_TEST
constexpr size_t SGTEST_CDC_CAPTURE_CAPACITY = 64;
uint8_t _sgtest_cdcBytes[SGTEST_CDC_CAPTURE_CAPACITY];
size_t  _sgtest_cdcCount    = 0;
bool    _sgtest_cdcOverflow = false;
#endif

void _writeCdc(uint8_t b) {
#ifdef SIMGATEWAY_TEST
    if (_sgtest_cdcCount < SGTEST_CDC_CAPTURE_CAPACITY) {
        _sgtest_cdcBytes[_sgtest_cdcCount++] = b;
    } else {
        _sgtest_cdcOverflow = true;
    }
#endif
    Serial.write(b);
}

// Process one UART byte through the state machine.
// Returns true if a HID setter fired this byte.
bool _processByte(uint8_t b) {
    switch (_state) {

        case ParserState::IDLE:
            if (b == 0xAA) {
                _state = ParserState::GOT_AA;
            } else {
                _writeCdc(b); // DCS-BIOS byte → CDC
            }
            return false;

        case ParserState::GOT_AA:
            if (b == 0x55) {
                _framePos = 0;
                _state    = ParserState::IN_FRAME;
            } else {
                // Resync: 0xAA was not magic; forward both bytes and resume.
                _writeCdc(0xAA);
                _writeCdc(b);
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
            for (auto* hat = OpenSkyhawk::HIDHatSwitch::head(); hat; hat = hat->next()) {
                if (hat->controlId() == controlId) { hat->dispatch(value); fired = true; }
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

void setup(SerialUART& uart, uint8_t txPin, uint8_t rxPin) {
    _uart = &uart;

#ifndef SIMGATEWAY_TEST
    // USB identity must be set before TinyUSB begins enumerating.
    TinyUSBDevice.setID(0x2E8A, 0x4134);
    TinyUSBDevice.setManufacturerDescriptor("OpenSkyhawk");
    TinyUSBDevice.setProductDescriptor("A-4E Skyhawk");
    Serial.begin(250000); // start Adafruit_USBD_CDC; required before available()/write() work
                          // (baud arg ignored by USB CDC; set to nominal 250000 to match docs)
    // Name the CDC interface (iInterface) so the serial port is identifiable by name, not just
    // VID/PID + CDC class. Must follow Serial.begin(), which otherwise leaves the library
    // default "TinyUSB Serial".
    Serial.setStringDescriptor("A-4E Skyhawk DCS-BIOS");
#endif

    _uart->setTX(txPin);
    _uart->setRX(rxPin);
    _uart->begin(250000);

    _hidBegin(); // no-op in SIMGATEWAY_TEST builds

#ifndef SIMGATEWAY_TEST
    Serial.println(F("=============================="));
    Serial.println(F("  SimGateway"));
    Serial.println(F("=============================="));
#endif
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
    if (anyFired) _hidSend();
}

#ifdef SIMGATEWAY_TEST
bool feedByte(uint8_t b)  { return _processByte(b); }
void resetParser()         { _state = ParserState::IDLE; _framePos = 0; }
void resetCdcCapture()     { _sgtest_cdcCount = 0; _sgtest_cdcOverflow = false; }
size_t cdcCaptureCount()   { return _sgtest_cdcCount; }
uint8_t cdcCaptureByte(size_t index) {
    return (index < _sgtest_cdcCount) ? _sgtest_cdcBytes[index] : 0;
}
bool cdcCaptureOverflow()  { return _sgtest_cdcOverflow; }
#endif

} // namespace SimGateway

#endif // ARDUINO_ARCH_RP2040
```


