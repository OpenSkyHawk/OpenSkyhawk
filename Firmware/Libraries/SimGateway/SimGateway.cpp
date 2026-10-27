#ifdef ARDUINO_ARCH_RP2040

#include "SimGateway.h"
#include <FirmwareVersion.h>  // OPENSKYHAWK_FW_* — the release version HELLO_ACK reports
#include "Uploader.h"         // the firmware uploader (#366)

// ── TinyUSB HID backend (production builds only) ──────────────────────────────
//
// SIMGATEWAY_TEST builds substitute no-op stubs below. The #ifndef guard prevents
// Adafruit_TinyUSB.h from being included in test builds, keeping tests free of USB
// enumeration side effects.

#ifndef SIMGATEWAY_TEST
#include <Adafruit_TinyUSB.h>
#include <EEPROM.h>
#include "hardware/structs/uart.h" // uart0_hw — PL011 registers; also used by the status LED
#include "hardware/regs/uart.h"    // UART_UARTRSR_*_BITS error-flag masks
#include "hardware/watchdog.h"     // watchdog_enable_caused_reboot(), watchdog_hw->scratch[]
#include "hardware/irq.h"          // user_irq_is_claimed() — the TinyUSB task IRQ (#389)

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
// Post-calibration capture. The production _hidSetAxis writes into a private HID report with
// no read path, so without this the transform HIDAxis::dispatch() applies is unobservable —
// the frame-parser capture globals below record the *pre*-transform value read off the wire.
int16_t _sgtest_axisValue = 0;
uint8_t _sgtest_axisIndex = 0xFF;
// One report per drain is the batching contract. Nothing else counts report sends, so without
// this a fork that called _hidSend() itself would be invisible to every test.
uint8_t _sgtest_sendCount = 0;

static void _hidBegin()                              {}
static void _hidSetAxis(uint8_t axisIndex, int16_t value) {
    _sgtest_axisIndex = axisIndex;
    _sgtest_axisValue = value;
}
static void _hidSetButton(uint8_t, bool)             {}
static void _hidSetHat(uint8_t, uint8_t)             {}
static void _hidSend()                               { _sgtest_sendCount++; }
} // anonymous namespace

#endif // SIMGATEWAY_TEST

// ── Test capture globals ───────────────────────────────────────────────────────

#ifdef SIMGATEWAY_TEST
uint16_t _sgtest_lastControlId = 0;
uint16_t _sgtest_lastValue     = 0;
uint8_t  _sgtest_dispatchCount = 0;
#endif

// ── Axis calibration state ────────────────────────────────────────────────────

namespace {

// The live calibration, indexed by HID axis index. Deliberately a file-static POD rather
// than per-object members: the C runtime zeroes .bss before any dynamic initialiser runs,
// so every HIDAxis constructed at static init already sees an all-zero blob, which fails
// axisCalValid() and yields identity. That removes the static-init ordering hazard instead
// of managing it — there is no window in which an axis could divide by zero.
//
// Loaded from flash by SimGateway::setup(). Never a constructor argument; see SimGateway.h.
OpenSkyhawk::CalBlob _calBlob;

#ifndef SIMGATEWAY_TEST
// Read the stored blob into RAM, falling back to "uncalibrated" on anything unexpected.
//
// A rejected blob is cleared in RAM only — flash is left exactly as found, so a version the
// firmware does not recognise survives a downgrade instead of being overwritten by it. The
// user's next commit is what replaces it.
//
// EEPROM here is the RP2040 core's flash-backed emulation: begin() allocates a RAM mirror and
// memcpys the sector into it, costing microseconds and no erase, so its position in setup() is
// a matter of tidiness rather than timing.
//
// Only get()/put() are used. getDataPtr() sets the dirty flag unconditionally — even for a
// pure read — which would force a real sector erase on every subsequent commit().
void _calLoad() {
    EEPROM.begin(256); // rounded up to a 256-byte multiple by the library; blob is 72
    EEPROM.get(0, _calBlob);
    if (!OpenSkyhawk::calBlobValid(_calBlob)) {
        OpenSkyhawk::calBlobClear(_calBlob);
    }
}
#else
// Test builds must never write flash, and must not depend on whatever a previous run left
// behind. Start every test from a known uncalibrated state.
void _calLoad() {
    OpenSkyhawk::calBlobClear(_calBlob);
}
#endif

} // namespace

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
uint8_t  HIDAxis::axisIndex() const        { return _axisIndex; }
HIDAxis* HIDAxis::next() const             { return _next; }
void     HIDAxis::dispatch(uint16_t value) {
    // Calibration first, offset last — the pipeline stays unsigned until the final step.
    const uint16_t v = (_axisIndex < AXIS_CAL_SLOTS)
                     ? axisCalApply(_calBlob.axes[_axisIndex], value)
                     : value;
    _hidSetAxis(_axisIndex, (int16_t)(v - 32768));
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

constexpr uint32_t UART_BAUD = 250000;

SerialUART*     _uart     = nullptr;
ParserState     _state    = ParserState::IDLE;
uint8_t         _frameBuf[4];
uint8_t         _framePos = 0;

#ifdef SIMGATEWAY_TEST
// 256 rather than 64: a CAL_DATA frame is 92 bytes, so the old capacity silently overflowed
// and surfaced as a confusing FAIL instead of an obvious error.
constexpr size_t SGTEST_CDC_CAPTURE_CAPACITY = 256;
uint8_t _sgtest_cdcBytes[SGTEST_CDC_CAPTURE_CAPACITY];
size_t  _sgtest_cdcCount    = 0;
bool    _sgtest_cdcOverflow = false;

// The UART side had no test seam. The calibration parser's whole safety property is that a
// rejected candidate is handed back to the relay byte-for-byte, and that is only assertable
// by capturing what reaches the UART.
constexpr size_t SGTEST_UART_CAPTURE_CAPACITY = 256;
uint8_t _sgtest_uartBytes[SGTEST_UART_CAPTURE_CAPACITY];
size_t  _sgtest_uartCount    = 0;
bool    _sgtest_uartOverflow = false;
#endif

// Every inbound byte that is not consumed by the calibration parser goes to PanelBridge
// through here. Mirrors _writeCdc's additive capture pattern: the tap records, the real
// write still happens.
void _writeUart(uint8_t b) {
#ifdef SIMGATEWAY_TEST
    if (_sgtest_uartCount < SGTEST_UART_CAPTURE_CAPACITY) {
        _sgtest_uartBytes[_sgtest_uartCount++] = b;
    } else {
        _sgtest_uartOverflow = true;
    }
#endif
    if (_uart) _uart->write(b);
}

// ── Host-bound output (#355) ──────────────────────────────────────────────────
//
// Nothing here may block. Adafruit_USBD_CDC::write() loops — calling only yield() — for as
// long as the host holds DTR but stops pulling IN data, and a loop() stalled there stops
// draining the UART, sending HID reports and animating the status LEDs. So every host-bound
// unit is written whole when the CDC FIFO has room for all of it, and dropped and counted
// otherwise. Truncating would be worse than dropping: half a DCS-BIOS command is a different
// command, and half a calibration frame is noise the client has to resynchronise past.

SimGateway::GatewayDiag _diag = {};

inline void _bump(uint16_t& c) { if (c != 0xFFFF) ++c; }

#ifdef SIMGATEWAY_TEST
int32_t _sgtest_cdcRoom = -1;   // < 0 = unlimited
#endif

uint16_t _cdcWritten = 0;   // bytes written since the stall detector last sampled

uint16_t _cdcRoom() {
#ifdef SIMGATEWAY_TEST
    return (_sgtest_cdcRoom < 0) ? (uint16_t)0xFFFF : (uint16_t)_sgtest_cdcRoom;
#else
    return (uint16_t)Serial.availableForWrite();
#endif
}

// The only writer to USB CDC. Mirrors _writeUart's additive capture pattern: the tap records
// what is actually written, and the real write still happens.
bool _writeCdcWhole(const uint8_t* p, uint16_t n) {
    if (_cdcRoom() < n) return false;
#ifdef SIMGATEWAY_TEST
    for (uint16_t i = 0; i < n; ++i) {
        if (_sgtest_cdcCount < SGTEST_CDC_CAPTURE_CAPACITY) {
            _sgtest_cdcBytes[_sgtest_cdcCount++] = p[i];
        } else {
            _sgtest_cdcOverflow = true;
        }
    }
#endif
    Serial.write(p, n);
    _cdcWritten = (uint16_t)(_cdcWritten + n);
    return true;
}

// The UART→host relay is line-granular. Everything PanelBridge sends the host is a DCS-BIOS
// message — `name arg\n`, printable ASCII — so a line is the unit that can be dropped
// without corrupting what follows. Binary on this path can only be line noise (or an 0xAA
// that began no HID frame), and relaying it would hand the host bytes nothing there parses.
uint8_t  _line[SimGateway::RELAY_LINE_MAX];
uint16_t _lineLen      = 0;
bool     _lineCorrupt  = false;
bool     _lineOverlong = false;
bool     _uploaderOwnsUart = false;   // last byte went to Uploader::onUartByte (#367)

void _lineReset() {
    _lineLen      = 0;
    _lineCorrupt  = false;
    _lineOverlong = false;
}

void _relayByte(uint8_t b) {
    // Noise before a line starts is dropped on its own: PanelBridge's UART coming up (every reset)
    // clocks a stray byte just ahead of its boot roster, and a line never starts with one, so
    // nothing real is lost (#367). Inside a line it still condemns the line.
    if (_lineLen == 0 && b != '\n' && (b < 0x20 || b > 0x7E)) return;
    if (_lineLen < SimGateway::RELAY_LINE_MAX) _line[_lineLen++] = b;
    else                                       _lineOverlong = true;

    if (b != '\n') {
        if (b < 0x20 || b > 0x7E) _lineCorrupt = true;
        return;
    }

    // The uploader reads PanelBridge's `_BOOT_RSP` and `_NODE_STATUS` lines (#366), and keeps the
    // bootloader's replies from the client while it flashes.
    const bool forUploader = !_lineOverlong && !_lineCorrupt
        && Uploader::onBridgeLine(reinterpret_cast<const char*>(_line), (uint16_t)(_lineLen - 1), millis());

    if      (_lineOverlong)                    _bump(_diag.linesOverlong);
    else if (_lineCorrupt)                     _bump(_diag.linesCorrupt);
    else if (forUploader)                      {}
    else if (!_writeCdcWhole(_line, _lineLen)) _bump(_diag.linesDropped);
    _lineReset();
}

// ── Calibration transport (issue #251) ────────────────────────────────────────
//
// Contract: FirmwarePlan/03-uart-usb-hid-protocol.md § Calibration Protocol (USB CDC).
//
// The parser runs at ALL times, not only during a session — it has to, because HELLO and
// GET_CAL are answered outside one. The property that keeps an always-on parser from eating
// the DCS stream is that a candidate is only consumed once it passes all three of magic,
// LEN-matches-TYPE, and CRC. Anything failing any of them is handed back to the relay
// byte-for-byte.

using OpenSkyhawk::CAL_FRAME_MAGIC;
using OpenSkyhawk::CAL_ENVELOPE_BYTES;
using OpenSkyhawk::CAL_MAX_FRAME;
using OpenSkyhawk::CAL_AXIS_NONE;

uint8_t  _calRx[OpenSkyhawk::CAL_MAX_INBOUND_FRAME];   // UPL_DATA (#366) is the one >92-byte frame, inbound only
uint16_t _calRxPos      = 0;

bool     _calSession    = false;
uint32_t _calLastRxMs   = 0;
uint8_t  _calStreamAxis = CAL_AXIS_NONE;  // axis whose RAW is streamed
uint8_t  _calRawSeq     = 0;              // free-running per-session counter for RAW

constexpr uint32_t CAL_SESSION_TIMEOUT_MS = 30000;

inline void _put16(uint8_t* p, uint16_t v) { p[0] = (uint8_t)(v & 0xFF); p[1] = (uint8_t)(v >> 8); }
inline uint16_t _get16(const uint8_t* p)   { return (uint16_t)(p[0] | ((uint16_t)p[1] << 8)); }
inline void _put32(uint8_t* p, uint32_t v) { _put16(p, (uint16_t)(v & 0xFFFF)); _put16(p + 2, (uint16_t)(v >> 16)); }

// Emit one frame to the host. Outbound is shared with DCS-BIOS text and _NODE_STATUS, so the
// client de-multiplexes on the raw chunk before line assembly. The relay buffers whole lines,
// so a frame can never land in the middle of one.
//
// Whole or not at all, like a relayed line. Only a host that has stopped reading loses one,
// and the client already treats an unanswered request as lost.
void _calSend(uint8_t type, uint8_t seq, const uint8_t* payload, uint16_t len) {
    uint8_t frame[CAL_MAX_FRAME];
    const uint16_t n = OpenSkyhawk::calBuildFrame(frame, sizeof(frame), type, seq, payload, len);
    if (n && !_writeCdcWhole(frame, n)) _bump(_diag.framesDropped);
}

void _calNack(uint8_t type, uint8_t seq, uint8_t reason, uint8_t detail) {
    uint8_t p[3];
    p[OpenSkyhawk::CAL_NACK_TYPE]   = type;
    p[OpenSkyhawk::CAL_NACK_REASON] = reason;
    p[OpenSkyhawk::CAL_NACK_DETAIL] = detail;
    _calSend(OpenSkyhawk::CAL_T_NACK, seq, p, sizeof(p));
}

// The whole ACK payload is the acknowledged type, so it is sent straight from `type`.
static_assert(OpenSkyhawk::CAL_ACK_TYPE == 0, "ACK payload is the type byte alone");
void _calAck(uint8_t type, uint8_t seq) {
    _calSend(OpenSkyhawk::CAL_T_ACK, seq, &type, 1);
}

// Initialised in CalHelloAckField order; the assert holds the list to the offsets.
static_assert(OpenSkyhawk::CAL_HELLO_ACK_PROTO == 0 && OpenSkyhawk::CAL_HELLO_ACK_BLOB_VERSION == 1
              && OpenSkyhawk::CAL_HELLO_ACK_AXIS_SLOTS == 2 && OpenSkyhawk::CAL_HELLO_ACK_FW_MAJOR == 3
              && OpenSkyhawk::CAL_HELLO_ACK_FW_MINOR == 4 && OpenSkyhawk::CAL_HELLO_ACK_FW_PATCH == 5,
              "CalHelloAckField moved: reorder _calSendHelloAck's initialiser to match");
void _calSendHelloAck(uint8_t seq) {
    const uint8_t p[6] = {
        OpenSkyhawk::CAL_PROTO_VERSION,
        (uint8_t)OpenSkyhawk::CAL_VERSION,
        OpenSkyhawk::AXIS_CAL_SLOTS,
        OPENSKYHAWK_FW_MAJOR, OPENSKYHAWK_FW_MINOR, OPENSKYHAWK_FW_PATCH,
    };
    _calSend(OpenSkyhawk::CAL_T_HELLO_ACK, seq, p, sizeof(p));
}

// Fixed 82 bytes: two bitmasks then eight 10-byte records, so the client reads it with one
// typed-array view. Absent slots carry controlId 0x0000 and zeroed endpoints.
//
// This reflects committed state by construction: COMMIT and RESET both persist immediately,
// so there is no pending copy that could disagree with flash.
void _calSendCalData(uint8_t seq) {
    uint8_t  p[82];
    for (size_t i = 0; i < sizeof(p); ++i) p[i] = 0;

    uint8_t  presentMask = 0, calibratedMask = 0;
    uint16_t ctrl[OpenSkyhawk::AXIS_CAL_SLOTS] = { 0 };

    for (auto* a = OpenSkyhawk::HIDAxis::head(); a; a = a->next()) {
        const uint8_t i = a->axisIndex();
        if (i < OpenSkyhawk::AXIS_CAL_SLOTS) {
            presentMask = (uint8_t)(presentMask | (1u << i));
            ctrl[i]     = a->controlId();      // last writer wins; the list allows duplicates
        }
    }
    for (uint8_t i = 0; i < OpenSkyhawk::AXIS_CAL_SLOTS; ++i) {
        if (axisCalValid(_calBlob.axes[i])) calibratedMask = (uint8_t)(calibratedMask | (1u << i));
    }

    p[OpenSkyhawk::CAL_DATA_PRESENT_MASK]    = presentMask;
    p[OpenSkyhawk::CAL_DATA_CALIBRATED_MASK] = calibratedMask;
    for (uint8_t i = 0; i < OpenSkyhawk::AXIS_CAL_SLOTS; ++i) {
        uint8_t* r = p + OpenSkyhawk::CAL_DATA_RECORDS + i * OpenSkyhawk::CAL_DATA_REC_BYTES;
        _put16(r + OpenSkyhawk::CAL_DATA_REC_CONTROL_ID, ctrl[i]);
        _put16(r + OpenSkyhawk::CAL_DATA_REC_MIN,        _calBlob.axes[i].min);
        _put16(r + OpenSkyhawk::CAL_DATA_REC_CENTRE,     _calBlob.axes[i].centre);
        _put16(r + OpenSkyhawk::CAL_DATA_REC_MAX,        _calBlob.axes[i].max);
        _put16(r + OpenSkyhawk::CAL_DATA_REC_DEADZONE,   _calBlob.axes[i].deadzone);
    }
    _calSend(OpenSkyhawk::CAL_T_CAL_DATA, seq, p, sizeof(p));
}

// Fixed CAL_DIAG_BYTES in CalDiagField order — the client reads it with one typed view (#355).
// Zero-initialised, so the reserved tail goes out as 0 (#295).
void _calSendDiag(uint8_t seq) {
    uint8_t p[OpenSkyhawk::CAL_DIAG_BYTES] = {};
    _put32(p + OpenSkyhawk::CAL_DIAG_UPTIME_MS,         millis());
    _put32(p + OpenSkyhawk::CAL_DIAG_FIRST_UART_ERR_MS, _diag.firstUartErrMs);
    _put32(p + OpenSkyhawk::CAL_DIAG_LAST_UART_ERR_MS,  _diag.lastUartErrMs);
    _put32(p + OpenSkyhawk::CAL_DIAG_FIRST_USB_LOSS_MS, _diag.firstUsbLossMs);
    _put32(p + OpenSkyhawk::CAL_DIAG_LAST_USB_LOSS_MS,  _diag.lastUsbLossMs);
    _put16(p + OpenSkyhawk::CAL_DIAG_UART_OVERRUN,      _diag.uartOverrun);
    _put16(p + OpenSkyhawk::CAL_DIAG_UART_FRAMING,      _diag.uartFraming);
    _put16(p + OpenSkyhawk::CAL_DIAG_UART_BREAK,        _diag.uartBreak);
    _put16(p + OpenSkyhawk::CAL_DIAG_UART_PARITY,       _diag.uartParity);
    _put16(p + OpenSkyhawk::CAL_DIAG_UART_RX_OVERFLOW,  _diag.uartRxOverflow);
    _put16(p + OpenSkyhawk::CAL_DIAG_LINES_DROPPED,     _diag.linesDropped);
    _put16(p + OpenSkyhawk::CAL_DIAG_LINES_CORRUPT,     _diag.linesCorrupt);
    _put16(p + OpenSkyhawk::CAL_DIAG_LINES_OVERLONG,    _diag.linesOverlong);
    _put16(p + OpenSkyhawk::CAL_DIAG_FRAMES_DROPPED,    _diag.framesDropped);
    _put16(p + OpenSkyhawk::CAL_DIAG_UART_RESTARTS,     _diag.uartRestarts);
    _put16(p + OpenSkyhawk::CAL_DIAG_USB_REATTACHES,    _diag.usbReattaches);
    _put16(p + OpenSkyhawk::CAL_DIAG_CDC_STALLS,        _diag.cdcStalls);
    p[OpenSkyhawk::CAL_DIAG_RECOVERY_REBOOTS] = _diag.recoveryReboots;
    p[OpenSkyhawk::CAL_DIAG_WATCHDOG_RESETS]  = _diag.watchdogResets;
    p[OpenSkyhawk::CAL_DIAG_RESET_REASON]     = _diag.resetReason;
    _calSend(OpenSkyhawk::CAL_T_DIAG, seq, p, sizeof(p));
}
static_assert(OpenSkyhawk::CAL_DIAG_RESET_REASON + 1 == OpenSkyhawk::CAL_DIAG_RESERVED,
              "CalDiagField grew: extend _calSendDiag to fill it");

// Write a candidate blob to flash and, only if that succeeds, make it the live calibration.
// Returns false if the storage layer refused.
//
// **This is the only place _calBlob is assigned after boot.** Handlers build a candidate from
// it, mutate the copy, and hand it over — so the live blob can never hold values that failed
// to persist. Mutating _calBlob first and repairing it afterwards would work too, but it
// leaves a window where the invariant is false and a repair path that a future handler can
// forget. Here there is nothing to forget.
//
// What that prevents: on a failed write the client is told NO_STORAGE — "nothing was written"
// — and the read-back it is instructed to perform must agree. If the candidate had already
// gone live, GET_CAL would report the new values as stored and the verification step would
// confirm the opposite of the truth. RESET is the sharper case: a failed RESET-all would
// silently drop every axis to uncalibrated while reporting that nothing happened, then
// restore them on the next power cycle.
//
// The erase runs with interrupts disabled for ~28 ms (measured), while the PL011's 32-entry
// RX FIFO fills in 1.28 ms at 250000 baud — so inbound UART bytes are lost and the overrun
// flag is set every single time. Without the recovery below, each successful save latches the
// FAULT LED red for its 2 s minimum hold, and a HID parser stranded mid-frame consumes the
// next four arriving bytes as a controlId/value pair.
//
// The failure branch has no automated coverage: EEPROM.commit() only returns false when
// begin() never ran or its allocation failed, and it is stubbed to true in test builds. That
// is why the invariant is structural rather than a repair — it cannot be regression-tested.
bool _calPersist(const OpenSkyhawk::CalBlob& candidate) {
    OpenSkyhawk::CalBlob staged = candidate;
    OpenSkyhawk::calBlobSeal(staged);

#ifndef SIMGATEWAY_TEST
    EEPROM.put(0, staged);
    const bool ok = EEPROM.commit();

    // Recovery, in order, and it runs whether or not the write succeeded — the erase may
    // have started regardless. The overrun is self-inflicted and expected, not a link
    // fault, so do not "fix" this RSR clear away — it also keeps the erase out of the
    // GET_DIAG overrun counter.
    while (_uart && _uart->available()) (void)_uart->read();
    uart0_hw->rsr = 0;
    _state    = ParserState::IDLE;
    _framePos = 0;
    _lineReset();   // the line in progress lost bytes to the overrun
#else
    const bool ok = true;   // test builds never touch flash
#endif

    if (ok) _calBlob = staged;
    return ok;
}

// Stream one sample of the axis under calibration.
//
// Only the selected axis streams. The dialog calibrates one axis at a time — which is why
// SESSION_OPEN names one and STREAM_SELECT switches it — so streaming the other seven would be
// bytes nobody is looking at. Every other axis still dispatches to HID exactly as normal; it
// simply emits no RAW.
//
// Carries both values from the same sample: `raw` as read off the wire, and `cal` after the
// stored calibration. That is what lets the dialog show the sensor reading beside what DCS is
// receiving without the client reimplementing the transform. Note `cal` reflects the calibration
// the device currently *holds* — there is no pending state, so it does not preview the endpoints
// being captured.
//
// **Droppable by design**, like every host-bound frame: _calSend() writes it whole or drops it
// when the CDC buffer is short. RAW is the type that actually gets dropped in practice, because
// it is the only one sent unasked.
//
// The sequence counter advances for every sample that *would* have been sent, including dropped
// ones. That gap is the only way a client can distinguish lost samples from an axis that stopped
// moving — the stream is genuinely silent at rest.
void _calEmitRaw(uint8_t axisIndex, uint16_t raw) {
    if (!_calSession || axisIndex != _calStreamAxis) return;

    const uint8_t seq = _calRawSeq++;

    const uint16_t cal = (axisIndex < OpenSkyhawk::AXIS_CAL_SLOTS)
                       ? axisCalApply(_calBlob.axes[axisIndex], raw)
                       : raw;

    uint8_t p[5];
    p[OpenSkyhawk::CAL_RAW_IDX] = axisIndex;
    _put16(p + OpenSkyhawk::CAL_RAW_RAW, raw);
    _put16(p + OpenSkyhawk::CAL_RAW_CAL, cal);
    _calSend(OpenSkyhawk::CAL_T_RAW, seq, p, sizeof(p));
}

void _calEndSession() {
    _calSession    = false;
    _calStreamAxis = CAL_AXIS_NONE;
    _calRawSeq     = 0;
}

// One axis per COMMIT, never a batch. The framing gate has already fixed len at 9.
//
// An earlier draft carried a count and up to eight records. Dropping it removes the only
// variable-length type — so "one legal length per type" holds with no exception — and closes
// a hazard the batch form had: nothing stopped a batch naming the same axis twice with
// different values, and the handler would silently apply the last one. Committing the axis
// the user just finished also matches the capture flow, where only one axis streams at a time,
// and keeps a bad axis from discarding good ones alongside it.
void _calHandleCommit(uint8_t seq, const uint8_t* pay, uint16_t len) {
    (void)len;
    const uint8_t idx = pay[OpenSkyhawk::CAL_COMMIT_IDX];

    if (idx >= OpenSkyhawk::AXIS_CAL_SLOTS) {
        _calNack(OpenSkyhawk::CAL_T_COMMIT, seq, OpenSkyhawk::CAL_NACK_BAD_INDEX, idx);
        return;
    }
    if (_get16(pay + OpenSkyhawk::CAL_COMMIT_DEADZONE) != 0) {
        _calNack(OpenSkyhawk::CAL_T_COMMIT, seq, OpenSkyhawk::CAL_NACK_BAD_DEADZONE, idx);
        return;
    }
    const OpenSkyhawk::AxisCal axis = {
        _get16(pay + OpenSkyhawk::CAL_COMMIT_MIN), _get16(pay + OpenSkyhawk::CAL_COMMIT_CENTRE),
        _get16(pay + OpenSkyhawk::CAL_COMMIT_MAX), _get16(pay + OpenSkyhawk::CAL_COMMIT_DEADZONE)
    };
    if (!axisCalValid(axis)) {
        _calNack(OpenSkyhawk::CAL_T_COMMIT, seq, OpenSkyhawk::CAL_NACK_BAD_ORDER, idx);
        return;
    }

    // Applied to a copy — the live blob changes only once the write has succeeded.
    OpenSkyhawk::CalBlob cand = _calBlob;
    cand.axes[idx] = axis;

    if (_calPersist(cand)) _calAck(OpenSkyhawk::CAL_T_COMMIT, seq);
    else _calNack(OpenSkyhawk::CAL_T_COMMIT, seq, OpenSkyhawk::CAL_NACK_NO_STORAGE, CAL_AXIS_NONE);
}

void _calHandleFrame(const uint8_t* f, uint16_t n) {
    const uint8_t  type = f[4];
    const uint8_t  seq  = f[5];
    const uint16_t len  = _get16(f + 6);
    const uint8_t* pay  = f + 8;
    (void)n;

    _calLastRxMs = millis();
    Uploader::onClientFrame(_calLastRxMs);

    // The firmware uploader's messages (#366). One session at a time: no upload while calibrating.
    // FLASH is refused too, not only BEGIN: with nothing staged it flashes the bundled bridge
    // image (#367), which would start a flash mid-calibration (#402).
    if (Uploader::handles(type)) {
        if (_calSession && (type == OpenSkyhawk::CAL_T_UPL_BEGIN || type == OpenSkyhawk::CAL_T_UPL_FLASH)) {
            _calNack(type, seq, OpenSkyhawk::CAL_NACK_BUSY, CAL_AXIS_NONE);
            return;
        }
        Uploader::onFrame(type, seq, pay, len, _calLastRxMs);
        return;
    }

    // Session gating. HELLO / GET_CAL / GET_DIAG / KEEPALIVE are answered at any time — that is what
    // lets the client show badges without opening a dialog.
    switch (type) {
        case OpenSkyhawk::CAL_T_COMMIT:
        case OpenSkyhawk::CAL_T_RESET:
        case OpenSkyhawk::CAL_T_STREAM_SELECT:
            if (!_calSession) {
                _calNack(type, seq, OpenSkyhawk::CAL_NACK_NO_SESSION, CAL_AXIS_NONE);
                return;
            }
            break;
        default: break;
    }

    switch (type) {
        case OpenSkyhawk::CAL_T_HELLO:
            _calSendHelloAck(seq);
            break;

        case OpenSkyhawk::CAL_T_GET_CAL:
            _calSendCalData(seq);
            break;

        case OpenSkyhawk::CAL_T_GET_DIAG:
            _calSendDiag(seq);
            break;

        case OpenSkyhawk::CAL_T_SESSION_OPEN: {
            if (Uploader::busy()) {   // an upload session or a flash owns the channel (#366)
                _calNack(type, seq, OpenSkyhawk::CAL_NACK_BUSY, CAL_AXIS_NONE);
                break;
            }
            const uint8_t axis = pay[OpenSkyhawk::CAL_SESSION_OPEN_AXIS_IDX];
            if (axis != CAL_AXIS_NONE && axis >= OpenSkyhawk::AXIS_CAL_SLOTS) {
                _calNack(type, seq, OpenSkyhawk::CAL_NACK_BAD_INDEX, axis);
                break;
            }
            _calSession    = true;
            _calStreamAxis = axis;
            _calRawSeq     = 0;
            uint8_t p[5];
            uint8_t* t = p + OpenSkyhawk::CAL_SESSION_ACK_TIMEOUT_MS;   // u32 LE as two u16
            _put16(t + 0, (uint16_t)(CAL_SESSION_TIMEOUT_MS & 0xFFFF));
            _put16(t + 2, (uint16_t)(CAL_SESSION_TIMEOUT_MS >> 16));
            p[OpenSkyhawk::CAL_SESSION_ACK_AXIS_IDX] = axis;
            _calSend(OpenSkyhawk::CAL_T_SESSION_ACK, seq, p, sizeof(p));
            break;
        }

        case OpenSkyhawk::CAL_T_STREAM_SELECT: {
            const uint8_t axis = pay[OpenSkyhawk::CAL_STREAM_SELECT_AXIS_IDX];
            if (axis != CAL_AXIS_NONE && axis >= OpenSkyhawk::AXIS_CAL_SLOTS) {
                _calNack(type, seq, OpenSkyhawk::CAL_NACK_BAD_INDEX, axis);
                break;
            }
            _calStreamAxis = axis;
            _calAck(type, seq);
            break;
        }

        case OpenSkyhawk::CAL_T_SESSION_CLOSE:
            _calEndSession();
            _calAck(type, seq);
            break;

        case OpenSkyhawk::CAL_T_KEEPALIVE:
            _calAck(type, seq);
            break;

        case OpenSkyhawk::CAL_T_COMMIT:
            _calHandleCommit(seq, pay, len);
            break;

        case OpenSkyhawk::CAL_T_RESET: {
            const uint8_t idx = pay[OpenSkyhawk::CAL_RESET_IDX];
            if (idx != CAL_AXIS_NONE && idx >= OpenSkyhawk::AXIS_CAL_SLOTS) {
                _calNack(type, seq, OpenSkyhawk::CAL_NACK_BAD_INDEX, idx);
                break;
            }
            // Deleting calibration, not restoring a default profile: the axis reverts to
            // identity passthrough. Persists immediately — no following COMMIT needed.
            //
            // Cleared on a copy for the same reason as COMMIT, and it matters more here: a
            // failed RESET-all would otherwise drop every axis to uncalibrated in RAM while
            // telling the client nothing was written.
            OpenSkyhawk::CalBlob cand = _calBlob;
            if (idx == CAL_AXIS_NONE) {
                for (uint8_t i = 0; i < OpenSkyhawk::AXIS_CAL_SLOTS; ++i)
                    cand.axes[i] = OpenSkyhawk::AxisCal{ 0, 0, 0, 0 };
            } else {
                cand.axes[idx] = OpenSkyhawk::AxisCal{ 0, 0, 0, 0 };
            }
            if (_calPersist(cand)) _calAck(type, seq);
            else _calNack(type, seq, OpenSkyhawk::CAL_NACK_NO_STORAGE, CAL_AXIS_NONE);
            break;
        }

        default:
            // Unreachable: calLenValidForType() rejects unknown types before the payload is
            // buffered, so this cannot be entered from the wire. Kept as a belt-and-braces
            // answer rather than a silent drop.
            _calNack(type, seq, OpenSkyhawk::CAL_NACK_BAD_TYPE, CAL_AXIS_NONE);
            break;
    }
}

// Give every held byte back to the relay, in order, and start scanning fresh.
void _calFlushHeld() {
    if (!Uploader::ownsUart())   // a flash owns the UART (#366): the client's bytes are dropped
        for (uint16_t i = 0; i < _calRxPos; ++i) _writeUart(_calRx[i]);
    _calRxPos = 0;
}

// One inbound byte. Returns true if this function took responsibility for it — either it is
// being held as part of a candidate, it belonged to a consumed frame, or it was handed back
// to the relay here. False means the caller should relay it.
bool _calInboundByte(uint8_t b) {
    // Magic phase.
    if (_calRxPos < 4) {
        if (b == CAL_FRAME_MAGIC[_calRxPos]) { _calRx[_calRxPos++] = b; return true; }
        _calFlushHeld();
        // The mismatching byte may itself begin a new candidate — 0xAA appears only at
        // index 0 of the magic, so this single re-test is enough to be lossless.
        if (b == CAL_FRAME_MAGIC[0]) { _calRx[_calRxPos++] = b; return true; }
        return false;
    }

    _calRx[_calRxPos++] = b;

    // Header complete: check LEN against TYPE *before* buffering any payload, so a false
    // magic followed by noise cannot make us wait for up to 65535 bytes.
    if (_calRxPos == 8) {
        if (!OpenSkyhawk::calLenValidForType(_calRx[4], _get16(_calRx + 6))) _calFlushHeld();
        return true;
    }
    if (_calRxPos < 8) return true;

    const uint16_t total = (uint16_t)(CAL_ENVELOPE_BYTES + _get16(_calRx + 6));
    if (_calRxPos < total) return true;

    if (OpenSkyhawk::calFrameCrcOk(_calRx, total)) {
        _calHandleFrame(_calRx, total);
        _calRxPos = 0;
    } else {
        _calFlushHeld();   // never was a frame — the relay gets its bytes back
    }
    return true;
}

// Process one UART byte through the state machine.
// Returns true if a HID setter fired this byte.
bool _processByte(uint8_t b) {
    switch (_state) {

        case ParserState::IDLE:
            if (b == 0xAA) {
                _state = ParserState::GOT_AA;
            } else {
                _relayByte(b); // DCS-BIOS byte → host-bound line
            }
            return false;

        case ParserState::GOT_AA:
            if (b == 0x55) {
                _framePos = 0;
                _state    = ParserState::IN_FRAME;
            } else {
                // Resync: 0xAA was not magic; both bytes go to the line and resume. The line they
                // land in is dropped as corrupt rather than relayed — marked here, since an 0xAA at
                // a line's start would otherwise be dropped as noise on its own (#367).
                _lineCorrupt = true;
                _relayByte(0xAA);
                _relayByte(b);
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
                if (a->controlId() == controlId) {
                    // Forking here rather than beside the controlId decode gives the axis
                    // index for free and emits nothing for button or hat frames. It must not
                    // touch `fired` or send a HID report: one report per drain is the
                    // contract, and breaking it only while calibrating would change what DCS
                    // sees mid-session.
                    _calEmitRaw(a->axisIndex(), value);
                    a->dispatch(value);
                    fired = true;
                }
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
constexpr uint32_t FAULT_RESTART_MS  = 3000; // latched this long → restart the UART, clear FAULT
constexpr uint32_t FAULT_RECENT_MS   = 30000; // a UART error this recent makes "suspended" unhealthy
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
uint32_t _faultSinceMs  = 0;   // when the current latch was set

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
        if (!_faultLatched) _faultSinceMs = now;
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

// FAULT self-clear (#355). Every observed FAULT came with the host link lost for good, and a
// silent bus would otherwise hold the latch forever — PanelBridge sends almost nothing with no
// DCS running, so "clean data resumes" may never happen. After FAULT_RESTART_MS the caller
// restarts the UART and the latch is cleared; errors that persist re-latch on the next tick,
// so a genuinely bad link keeps showing FAULT, restarting every FAULT_RESTART_MS. The event
// itself is never lost: GatewayDiag keeps the counts and times.
bool _faultRestartCheck(uint32_t now) {
    if (!_faultLatched || (uint32_t)(now - _faultSinceMs) < FAULT_RESTART_MS) return false;
    _faultLatched = false;
    _bump(_diag.uartRestarts);
    return true;
}

// Fold one tick's PL011 error bits into the counters. Separate bits, not one flag: overrun
// (the UART IRQ was held off) and framing/break (electrical) point at different causes.
void _recordUartErrors(uint32_t now, bool oe, bool fe, bool be, bool pe) {
    if (!(oe || fe || be || pe)) return;
    if (oe) _bump(_diag.uartOverrun);
    if (fe) _bump(_diag.uartFraming);
    if (be) _bump(_diag.uartBreak);
    if (pe) _bump(_diag.uartParity);
    const uint32_t t = now ? now : 1;   // 0 is reserved for "never"
    if (_diag.firstUartErrMs == 0) _diag.firstUartErrMs = t;
    _diag.lastUartErrMs = t;
}

// Restart the PanelBridge UART: drops the queue, clears the sticky error flags, and starts the
// parser and the host-bound line from scratch.
void _uartRestart() {
#ifndef SIMGATEWAY_TEST
    if (_uart) {
        _uart->end();
        _uart->begin(UART_BAUD);   // the RX queue size set in setup() persists across end()
    }
    uart0_hw->rsr = 0;
#endif
    _state    = ParserState::IDLE;
    _framePos = 0;
    _lineReset();
}

// ── USB recovery ladder (#355) ────────────────────────────────────────────────
//
// The decision logic is pure and lives in LinkRecovery.cpp. This part samples the inputs,
// records the forensics, and applies the action.
//
// "Healthy" is mounted, not suspended while a UART fault is recent, and the CDC IN endpoint not
// stalled. Mounted is the main signal — a host that sleeps keeps the device mounted. Suspend
// counts only alongside a fault because a sleeping host also suspends the bus, and that must not
// trigger a re-attach. The stall covers the case mounted() cannot see: still configured, but the
// host stopped polling, so every line is dropped and loop() — and the watchdog — run on happily.

OpenSkyhawk::LinkRecovery _link = {};
bool _linkWasHealthy = false;   // false at boot, so enumeration is not a "loss"
#ifndef SIMGATEWAY_TEST
bool _wdtArmed = false;

// TinyUSB's rp2040 port also runs tud_task() from a user IRQ that every USB interrupt pends. In
// interrupt context that can wait forever (#389): yield()'s CDC flush (TinyUSB_Device_FlushCDC →
// tud_cdc_n_write_flush) holds TinyUSB's endpoint mutex in the main thread, data from the host
// arrives, and the IRQ's CDC callback blocks on that mutex — with its owner preempted beneath it.
// The watchdog then reboots the board 3 s later. Seen under heavy host traffic on the bench and
// caught by PC sampling. yield() already runs TinyUSB_Device_Task() after every loop(), so the
// IRQ path is redundant: switch it off and USB is serviced from the main thread only. SimGateway
// claims no user IRQ of its own, so a claimed, enabled user IRQ with a handler is TinyUSB's.
void _usbTaskFromLoopOnly() {
    for (uint n = FIRST_USER_IRQ; n < NUM_IRQS; ++n) {
        if (user_irq_is_claimed(n) && irq_is_enabled(n) && irq_get_exclusive_handler(n))
            irq_set_enabled(n, false);
    }
}
#endif

// CDC stall detector. Progress is "something drained since the last sample": free space now,
// plus what we wrote in between, exceeds the free space last time. Room alone would miss a link
// that drains exactly as fast as we refill it. Nothing pending (room == capacity) or no host
// (DTR low) is never a stall — an idle port must not trip the ladder.
uint32_t _cdcProgressMs = 0;
uint16_t _cdcLastRoom   = 0;
bool     _cdcStalled    = false;

bool _cdcStallUpdate(uint32_t now, bool dtr, uint16_t room, uint16_t capacity, uint16_t written) {
    const bool drained = (uint32_t)room + written > _cdcLastRoom;
    if (!dtr || room >= capacity || drained) _cdcProgressMs = now;
    _cdcLastRoom = room;

    const bool stalled = (uint32_t)(now - _cdcProgressMs) >= SimGateway::CDC_STALL_MS;
    if (stalled && !_cdcStalled) _bump(_diag.cdcStalls);
    _cdcStalled = stalled;
    return stalled;
}

OpenSkyhawk::LinkAction _linkUpdate(uint32_t now, bool mounted, bool suspended, bool cdcStalled) {
    const bool faultRecent = _diag.lastUartErrMs != 0 &&
                             (uint32_t)(now - _diag.lastUartErrMs) < FAULT_RECENT_MS;
    const bool healthy = mounted && !(suspended && faultRecent) && !cdcStalled;

    if (_linkWasHealthy && !healthy) {
        const uint32_t t = now ? now : 1;
        if (_diag.firstUsbLossMs == 0) _diag.firstUsbLossMs = t;
        _diag.lastUsbLossMs = t;
    }
    _linkWasHealthy = healthy;

    const OpenSkyhawk::LinkAction a = OpenSkyhawk::linkRecoveryStep(_link, now, healthy);
    if (a == OpenSkyhawk::LinkAction::DETACH) _bump(_diag.usbReattaches);
    return a;
}

// Boot forensics live in watchdog scratch registers 0–3, which the SDK leaves to the
// application (it uses 4–7 for its own reboot handshake). They survive a watchdog reboot and
// are cleared by power-on, so the magic word tells a warm boot from a cold one.
constexpr uint32_t SCRATCH_MAGIC    = 0x53474457;   // "SGDW"
constexpr uint32_t PENDING_RECOVERY = 0x52454356;   // "RECV" — set just before a ladder reboot
enum : uint8_t { SCR_MAGIC = 0, SCR_RECOVERY_REBOOTS = 1, SCR_WATCHDOG_RESETS = 2, SCR_PENDING = 3 };

void _bootRecord() {
    bool afterRecovery = false;
#ifndef SIMGATEWAY_TEST
    io_rw_32* const sc = watchdog_hw->scratch;
    if (sc[SCR_MAGIC] != SCRATCH_MAGIC) {
        sc[SCR_MAGIC]            = SCRATCH_MAGIC;
        sc[SCR_RECOVERY_REBOOTS] = 0;
        sc[SCR_WATCHDOG_RESETS]  = 0;
        sc[SCR_PENDING]          = 0;
    }
    afterRecovery     = (sc[SCR_PENDING] == PENDING_RECOVERY);
    sc[SCR_PENDING]   = 0;
    if (afterRecovery) {
        _diag.resetReason = OpenSkyhawk::CAL_DIAG_RESET_RECOVERY;
    } else if (watchdog_enable_caused_reboot()) {
        _diag.resetReason = OpenSkyhawk::CAL_DIAG_RESET_WATCHDOG;
        if (sc[SCR_WATCHDOG_RESETS] < 0xFF) sc[SCR_WATCHDOG_RESETS] = sc[SCR_WATCHDOG_RESETS] + 1;
    } else {
        _diag.resetReason = OpenSkyhawk::CAL_DIAG_RESET_POWER_ON;
    }
    _diag.recoveryReboots = (uint8_t)sc[SCR_RECOVERY_REBOOTS];
    _diag.watchdogResets  = (uint8_t)sc[SCR_WATCHDOG_RESETS];
#endif
    OpenSkyhawk::linkRecoveryBegin(_link, afterRecovery);
}

#ifndef SIMGATEWAY_TEST
void _linkTick(uint32_t now) {
    const bool stalled = _cdcStallUpdate(now, Serial.dtr() != 0, (uint16_t)Serial.availableForWrite(),
                                         CFG_TUD_CDC_TX_BUFSIZE, _cdcWritten);
    _cdcWritten = 0;
    switch (_linkUpdate(now, TinyUSBDevice.mounted(), TinyUSBDevice.suspended(), stalled)) {
        case OpenSkyhawk::LinkAction::DETACH: TinyUSBDevice.detach(); break;
        case OpenSkyhawk::LinkAction::ATTACH: TinyUSBDevice.attach(); break;
        case OpenSkyhawk::LinkAction::REBOOT: {
            io_rw_32* const sc = watchdog_hw->scratch;
            if (sc[SCR_RECOVERY_REBOOTS] < 0xFF) sc[SCR_RECOVERY_REBOOTS] = sc[SCR_RECOVERY_REBOOTS] + 1;
            sc[SCR_PENDING] = PENDING_RECOVERY;
            rp2040.reboot();   // does not return
            break;
        }
        default: break;
    }
}
#endif

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
    _bootRecord();   // first: reads the reboot reason before anything can overwrite it

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
    _uart->setFIFOSize(UART_RX_QUEUE_BYTES);   // before begin(): the core allocates it there
    _uart->begin(UART_BAUD);

    _calLoad(); // RAM-only in SIMGATEWAY_TEST builds — tests never touch flash

    Uploader::begin({
        [](uint8_t type, uint8_t seq, const uint8_t* p, uint16_t n) { _calSend(type, seq, p, n); },
        [](uint8_t type, uint8_t seq) { _calAck(type, seq); },
        [](uint8_t type, uint8_t seq, uint8_t reason, uint8_t detail) { _calNack(type, seq, reason, detail); },
        [](const uint8_t* p, uint16_t n) { for (uint16_t i = 0; i < n; ++i) _writeUart(p[i]); },
    });

    _hidBegin(); // no-op in SIMGATEWAY_TEST builds

    statusLedBegin(); // configure GP2/GP3 status LEDs (both off)

#ifndef SIMGATEWAY_TEST
    // Whole or not at all, like everything else host-bound — usually no host has the port
    // open yet, and then it goes nowhere either way.
    static const char banner[] =
        "==============================\r\n"
        "  SimGateway\r\n"
        "==============================\r\n";
    (void)_writeCdcWhole(reinterpret_cast<const uint8_t*>(banner), sizeof(banner) - 1);
#endif
}

const GatewayDiag& diagnostics() { return _diag; }

const OpenSkyhawk::CalBlob& calibration() { return _calBlob; }

void loop() {
#ifndef SIMGATEWAY_TEST
    // 0. Hardware watchdog. Armed here rather than in setup() so a sketch may still block in
    //    its own setup() (test sketches wait for the host) without being reset for it.
    if (_wdtArmed) {
        rp2040.wdt_reset();
    } else {
        rp2040.wdt_begin(WATCHDOG_MS);
        _wdtArmed = true;
        _usbTaskFromLoopOnly();   // with the watchdog: from here on loop() runs, so yield() services USB
    }
#endif

    // 1. Forward CDC → UART (PC DCS-BIOS stream to PanelBridge).
    //    Bytes moving here is the "host is talking" signal that drives STREAMING.
    //    Every byte passes the calibration parser first. It consumes only complete, valid
    //    frames; anything else reaches PanelBridge untouched, including the bytes of a
    //    rejected candidate. Cost is a comparison per byte against a UART that is three
    //    orders of magnitude slower.
    //    Capped per call: the UART write blocks once its FIFO is full, and an uncapped drain of
    //    an export burst would keep step 2 from running for tens of milliseconds.
    bool cdcMoved = false;
    uint16_t budget = CDC_TO_UART_BUDGET;
    while (budget && Serial.available()) {
        const uint8_t b = (uint8_t)Serial.read();
        // While the uploader flashes it owns the UART (#366): the client's export bytes are dropped,
        // so a client that didn't pause its relay can't corrupt the transfer.
        if (!_calInboundByte(b) && !Uploader::ownsUart()) _writeUart(b);
        cdcMoved = true;
        --budget;
    }
    //    Calibration bytes still count as "the host is talking" — a session is host traffic
    //    even though none of it is forwarded.
    if (cdcMoved) _lastCdcRxMs = millis();

    // 2. Drain UART; HID frames dispatched, DCS-BIOS bytes forwarded to CDC.
    //    UART RX moving (error-free) is the proof-of-recovery signal for FAULT.
    bool anyFired = false;
    while (_uart->available()) {
        const uint8_t b = (uint8_t)_uart->read();
        // While it flashes the bridge the uploader reads the UART itself (#367). Each hand-over starts
        // the relay parsers clean: a line cut off at the switch would swallow the first one after it
        // — the bridge's boot _NODE_STATUS row, which is what confirms the flash.
        const bool owned = Uploader::ownsUartRx();
        if (owned != _uploaderOwnsUart) { _uploaderOwnsUart = owned; _state = ParserState::IDLE; _lineReset(); }
        if (owned) Uploader::onUartByte(b, millis());
        else       anyFired |= _processByte(b);
        _uartMovedThisTick = true;
    }

    // 3. Flush one HID report if any setter fired this iteration
    if (anyFired) _hidSend();

    // 3b. Expire an abandoned calibration session. Placed after the HID send so the joystick
    //     path is never delayed by it. An expired session degrades to exactly normal
    //     operation — the outbound direction was never taken over.
    if (_calSession && (uint32_t)(millis() - _calLastRxMs) > CAL_SESSION_TIMEOUT_MS) {
        _calEndSession();
    }

    // 3c. Advance a firmware upload (#366) — at most one block onto the UART per loop.
    Uploader::tick(millis());

    // 4. Advance the status-LED state machine (non-blocking). Reads the uart0 RSR
    //    after the drain so this tick's UART errors are visible, and counts them.
    statusTick();

    // 5. USB recovery ladder. After statusTick so it sees this tick's UART errors.
#ifndef SIMGATEWAY_TEST
    _linkTick(millis());
#endif
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
    const uint32_t rsr = STATUS_UART_HW->rsr & RSR_ERR;
    bool rsrError = rsr != 0;
    if (rsrError) STATUS_UART_HW->rsr = 0;
    _recordUartErrors(now, rsr & UART_UARTRSR_OE_BITS, rsr & UART_UARTRSR_FE_BITS,
                           rsr & UART_UARTRSR_BE_BITS, rsr & UART_UARTRSR_PE_BITS);

    // The core's UART IRQ moves bytes into a software queue; when loop() falls behind, that
    // queue overflows and drops bytes without touching RSR. overflow() reads and clears it.
    if (_uart && _uart->overflow()) _bump(_diag.uartRxOverflow);
#else
    bool mounted  = false; // test builds drive the state machine via the hooks below
    bool rsrError = false;
#endif

    if (mounted) _everMounted = true;
    bool faultActive   = _updateFaultLatch(now, rsrError, _uartMovedThisTick);
    _uartMovedThisTick = false;
    if (_faultRestartCheck(now)) {
        _uartRestart();
        faultActive = false;
    }

    StatusInputs in{ now, mounted, _lastCdcRxMs, _everMounted, faultActive };
    _applyLed(_resolveStatus(in));
}

#ifdef SIMGATEWAY_TEST
bool feedByte(uint8_t b)  { return _processByte(b); }
void resetParser()         { _state = ParserState::IDLE; _framePos = 0; _lineReset(); }
void cdcSetRoomForTest(int32_t room) { _sgtest_cdcRoom = room; }
void diagResetForTest()    { _diag = GatewayDiag{}; }
void diagUartErrorsForTest(uint32_t now, bool oe, bool fe, bool be, bool pe) {
    _recordUartErrors(now, oe, fe, be, pe);
}
OpenSkyhawk::LinkAction linkStepForTest(uint32_t now, bool mounted, bool suspended, bool cdcStalled) {
    return _linkUpdate(now, mounted, suspended, cdcStalled);
}
bool cdcStallStepForTest(uint32_t now, bool dtr, uint16_t room, uint16_t capacity, uint16_t written) {
    return _cdcStallUpdate(now, dtr, room, capacity, written);
}
void linkResetForTest(bool afterRecoveryReboot) {
    OpenSkyhawk::linkRecoveryBegin(_link, afterRecoveryReboot);
    _linkWasHealthy = false;
    _cdcProgressMs  = 0;
    _cdcLastRoom    = 0;
    _cdcStalled     = false;
}
void resetCdcCapture()     { _sgtest_cdcCount = 0; _sgtest_cdcOverflow = false; }
size_t cdcCaptureCount()   { return _sgtest_cdcCount; }
uint8_t cdcCaptureByte(size_t index) {
    return (index < _sgtest_cdcCount) ? _sgtest_cdcBytes[index] : 0;
}
bool cdcCaptureOverflow()  { return _sgtest_cdcOverflow; }

// Mirrors loop() step 1 exactly, relay included — a hook that only ran the parser would let
// a test pass while the bytes it declined went nowhere.
bool feedCdcByte(uint8_t b) {
    const bool absorbed = _calInboundByte(b);
    if (!absorbed) _writeUart(b);
    return absorbed;
}
void resetUartCapture()       { _sgtest_uartCount = 0; _sgtest_uartOverflow = false; }
size_t uartCaptureCount()     { return _sgtest_uartCount; }
uint8_t uartCaptureByte(size_t index) {
    return (index < _sgtest_uartCount) ? _sgtest_uartBytes[index] : 0;
}
bool uartCaptureOverflow()    { return _sgtest_uartOverflow; }
void calResetForTest()        { _calEndSession(); _calRxPos = 0; _calLastRxMs = 0; }
bool calSessionOpen()         { return _calSession; }
uint8_t calStreamAxis()       { return _calStreamAxis; }

void calSetForTest(const OpenSkyhawk::CalBlob& blob) { _calBlob = blob; }
int16_t lastAxisValue()    { return _sgtest_axisValue; }
uint8_t lastAxisIndex()    { return _sgtest_axisIndex; }
void resetAxisCapture()    { _sgtest_axisValue = 0; _sgtest_axisIndex = 0xFF; }
uint8_t hidSendCount()     { return _sgtest_sendCount; }
void resetHidSendCount()   { _sgtest_sendCount = 0; }

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

bool statusFaultRestartCheck(uint32_t now) { return _faultRestartCheck(now); }

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
    _faultSinceMs      = 0;
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
