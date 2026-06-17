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

// ── Status LED state machine ──────────────────────────────────────────────────
//
// Drives the two board-mounted SimGateway status LEDs (RED = GP3, GREEN = GP2,
// active-high) with a non-blocking millis() animator, ticked from loop().
//
// The state-selection + animation-phase logic is PURE: it takes `now` as a
// parameter, touches no hardware, and never calls millis() internally — so
// SIMGATEWAY_TEST builds unit-test it with injected inputs (statusInject /
// statusFaultStep). Only _applyLed() touches GPIO, and the TinyUSBDevice.mounted()
// poll + PL011 RSR read live behind #ifndef SIMGATEWAY_TEST.

#ifndef SIMGATEWAY_TEST
#include "hardware/structs/uart.h" // uart0_hw — PL011 registers
#include "hardware/regs/uart.h"    // UART_UARTRSR_*_BITS error-flag masks
#endif

namespace {

using SimGateway::Anim;
using SimGateway::LedState;

enum class LedColor : uint8_t { NONE, RED, GREEN };

constexpr uint8_t  PIN_LED_GREEN     = 2;    // GP2
constexpr uint8_t  PIN_LED_RED       = 3;    // GP3
constexpr uint32_t STREAM_WINDOW_MS  = 500;  // CDC-RX recency → STREAMING
constexpr uint32_t INIT_WINDOW_MS    = 2000; // boot grace before NO_HOST if never mounted
constexpr uint32_t FAULT_MIN_HOLD_MS = 2000; // FAULT visibility floor (~8 fast flashes)
constexpr uint32_t SLOW_PERIOD_MS    = 1000; // 1 Hz
constexpr uint32_t FAST_PERIOD_MS    = 250;  // 4 Hz
constexpr uint32_t ALT_PERIOD_MS     = 500;  // reserved
constexpr bool     ENABLE_TRAFFIC_PULSE = false; // STREAMING is plain SOLID per AC

struct StatusInputs {
    uint32_t now;
    bool     mounted;
    uint32_t lastCdcRxMs;
    bool     everMounted;
    bool     faultActive;
};

struct LedOutput {
    LedState state;
    LedColor color;
    Anim     anim;
    bool     redOn;
    bool     greenOn;
};

// Sampled signal state (updated by loop() / statusTick()).
uint32_t _lastCdcRxMs      = 0;
bool     _uartMovedThisTick = false;
bool     _everMounted      = false;

// FAULT latch state.
bool     _faultLatched  = false;
bool     _faultEverSeen = false;
uint32_t _lastFaultMs   = 0;
uint32_t _lastUartRxMs  = 0;

// Pure: pick the active state from sampled inputs (priority high → low).
LedState _selectState(const StatusInputs& in) {
    if (in.faultActive) return LedState::FAULT;
    if (!in.mounted && (in.everMounted || in.now >= INIT_WINDOW_MS)) return LedState::NO_HOST;
    if (in.mounted && (uint32_t)(in.now - in.lastCdcRxMs) <= STREAM_WINDOW_MS) return LedState::STREAMING;
    if (in.mounted) return LedState::USB_IDLE;
    return LedState::INIT;
}

// Pure: map a state to its colour + animation.
void _animFor(LedState s, LedColor& color, Anim& anim) {
    switch (s) {
        case LedState::FAULT:     color = LedColor::RED;   anim = Anim::FAST;  break;
        case LedState::NO_HOST:   color = LedColor::RED;   anim = Anim::SOLID; break;
        case LedState::STREAMING: color = LedColor::GREEN; anim = ENABLE_TRAFFIC_PULSE ? Anim::PULSE : Anim::SOLID; break;
        case LedState::USB_IDLE:  color = LedColor::GREEN; anim = Anim::SLOW;  break;
        case LedState::INIT:      color = LedColor::RED;   anim = Anim::SLOW;  break;
    }
}

// Pure: on/off for an animation at time `now` (50% duty for blinks).
bool _animOn(Anim anim, uint32_t now) {
    switch (anim) {
        case Anim::OFF:   return false;
        case Anim::SOLID: return true;
        case Anim::SLOW:  return (now % SLOW_PERIOD_MS) < (SLOW_PERIOD_MS / 2);
        case Anim::FAST:  return (now % FAST_PERIOD_MS) < (FAST_PERIOD_MS / 2);
        case Anim::ALT:   return (now % ALT_PERIOD_MS)  < (ALT_PERIOD_MS / 2);
        case Anim::PULSE: return true; // baseline solid; PULSE off-blip disabled by default
    }
    return false;
}

// Pure: resolve full LED output (state + colour + anim + pin levels) for `now`.
LedOutput _resolveStatus(const StatusInputs& in) {
    LedOutput out{};
    out.state = _selectState(in);
    _animFor(out.state, out.color, out.anim);
    bool on = _animOn(out.anim, in.now);
    out.redOn   = (out.color == LedColor::RED)   && on;
    out.greenOn = (out.color == LedColor::GREEN) && on;
    return out;
}

// Update the FAULT latch from this tick's signals. Shared by production sampling
// (statusTick) and the SIMGATEWAY_TEST statusFaultStep() hook.
//   rsrError    — a PL011 error bit was set this tick.
//   uartRxMoved — ≥1 error-free byte was read from the UART this tick.
// FAULT latches on any error and re-stamps while errors persist; it clears only
// when (a) ≥ FAULT_MIN_HOLD_MS has elapsed since the last error AND (b) an
// error-free byte arrived on the UART *after* that error. A silent bus therefore
// holds FAULT until clean data resumes. Returns the resolved faultActive flag.
bool _updateFaultLatch(uint32_t now, bool rsrError, bool uartRxMoved) {
    if (rsrError) {
        _faultEverSeen = true;
        _faultLatched  = true;
        _lastFaultMs   = now;
    } else if (uartRxMoved) {
        _lastUartRxMs = now;
    }
    if (_faultLatched &&
        (uint32_t)(now - _lastFaultMs) >= FAULT_MIN_HOLD_MS &&
        (int32_t)(_lastUartRxMs - _lastFaultMs) > 0) {
        _faultLatched = false;
    }
    return _faultLatched;
}

#ifdef SIMGATEWAY_TEST
bool      _sgtest_redLevel   = false;
bool      _sgtest_greenLevel = false;
StatusInputs _sgtest_inputs  = {};
LedOutput    _sgtest_out     = {};
#endif

// The only function that touches the LED GPIO (active-high). In test builds it
// captures the resolved levels instead of writing pins.
void _applyLed(const LedOutput& out) {
#ifdef SIMGATEWAY_TEST
    _sgtest_redLevel   = out.redOn;
    _sgtest_greenLevel = out.greenOn;
#else
    digitalWrite(PIN_LED_RED,   out.redOn   ? HIGH : LOW);
    digitalWrite(PIN_LED_GREEN, out.greenOn ? HIGH : LOW);
#endif
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

    statusLedBegin(); // configure GP2/GP3 status LEDs (both off)

#ifndef SIMGATEWAY_TEST
    Serial.println(F("=============================="));
    Serial.println(F("  SimGateway"));
    Serial.println(F("=============================="));
#endif
}

void loop() {
    // 1. Forward CDC → UART (PC DCS-BIOS stream to PanelBridge).
    //    Bytes moving here is the "host is talking" signal that drives STREAMING.
    bool cdcMoved = false;
    while (Serial.available()) {
        _uart->write(Serial.read());
        cdcMoved = true;
    }
    if (cdcMoved) _lastCdcRxMs = millis();

    // 2. Drain UART; HID frames dispatched, DCS-BIOS bytes forwarded to CDC.
    //    UART RX moving (error-free) is the proof-of-recovery signal for FAULT.
    bool anyFired = false;
    while (_uart->available()) {
        anyFired |= _processByte(_uart->read());
        _uartMovedThisTick = true;
    }

    // 3. Flush one HID report if any setter fired this iteration
    if (anyFired) _hidSend();

    // 4. Advance the status-LED state machine (non-blocking). Reads the uart0 RSR
    //    after the drain so this tick's UART errors are visible.
    statusTick();
}

void statusLedBegin() {
#ifndef SIMGATEWAY_TEST
    pinMode(PIN_LED_RED,   OUTPUT);
    pinMode(PIN_LED_GREEN, OUTPUT);
    digitalWrite(PIN_LED_RED,   LOW);
    digitalWrite(PIN_LED_GREEN, LOW);
#endif
}

void statusTick() {
    uint32_t now = millis();

#ifndef SIMGATEWAY_TEST
    bool mounted = TinyUSBDevice.mounted();

    // Read the PL011 sticky error flags. Serial1 == uart0 on this board
    // (SerialUART.cpp: `#define __SERIAL1_DEVICE uart0`); a future board wiring the
    // status UART to uart1 must change STATUS_UART_HW. Clear on every read
    // (write-to-clear via the ECR alias) or a single overrun pins FAULT forever.
    auto* const        STATUS_UART_HW = uart0_hw;
    constexpr uint32_t RSR_ERR = UART_UARTRSR_OE_BITS | UART_UARTRSR_BE_BITS |
                                 UART_UARTRSR_PE_BITS | UART_UARTRSR_FE_BITS;
    bool rsrError = (STATUS_UART_HW->rsr & RSR_ERR) != 0;
    if (rsrError) STATUS_UART_HW->rsr = 0;
#else
    bool mounted  = false; // test builds drive the state machine via the hooks below
    bool rsrError = false;
#endif

    if (mounted) _everMounted = true;
    bool faultActive   = _updateFaultLatch(now, rsrError, _uartMovedThisTick);
    _uartMovedThisTick = false;

    StatusInputs in{ now, mounted, _lastCdcRxMs, _everMounted, faultActive };
    _applyLed(_resolveStatus(in));
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

// ── Status-LED test hooks ─────────────────────────────────────────────────────
void statusInject(uint32_t now, bool mounted, uint32_t lastCdcRxMs, bool faultActive) {
    if (mounted) _everMounted = true;
    _sgtest_inputs = StatusInputs{ now, mounted, lastCdcRxMs, _everMounted, faultActive };
}

void statusResolve() {
    _sgtest_out = _resolveStatus(_sgtest_inputs);
    _applyLed(_sgtest_out);
}

bool statusFaultStep(uint32_t now, bool rsrError, bool uartRxMoved) {
    return _updateFaultLatch(now, rsrError, uartRxMoved);
}

LedState statusState()      { return _sgtest_out.state; }
Anim     statusAnim()       { return _sgtest_out.anim; }
bool     statusRedLevel()   { return _sgtest_redLevel; }
bool     statusGreenLevel() { return _sgtest_greenLevel; }

void statusResetForTest() {
    _everMounted       = false;
    _faultLatched      = false;
    _faultEverSeen     = false;
    _lastFaultMs       = 0;
    _lastUartRxMs      = 0;
    _lastCdcRxMs       = 0;
    _uartMovedThisTick = false;
    _sgtest_redLevel   = false;
    _sgtest_greenLevel = false;
    _sgtest_inputs     = StatusInputs{};
    _sgtest_out        = LedOutput{};
}
#endif

} // namespace SimGateway

#endif // ARDUINO_ARCH_RP2040
