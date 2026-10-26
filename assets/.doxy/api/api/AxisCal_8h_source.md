

# File AxisCal.h

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**SimGateway**](dir_a54aa0246e1c520ae49dfef506a428ca.md) **>** [**AxisCal.h**](AxisCal_8h.md)

[Go to the documentation of this file](AxisCal_8h.md)


```C++

#pragma once

#include <stddef.h>
#include <stdint.h>

namespace OpenSkyhawk {

struct AxisCal {
    uint16_t min;       
    uint16_t centre;    
    uint16_t max;       
    uint16_t deadzone;
};

constexpr uint8_t AXIS_CAL_SLOTS = 8;

constexpr uint32_t CAL_MAGIC = ((uint32_t)'O')
                             | ((uint32_t)'S' <<  8)
                             | ((uint32_t)'K' << 16)
                             | ((uint32_t)'C' << 24);

constexpr uint16_t CAL_VERSION = 1;

struct CalBlob {
    uint32_t magic;                  
    uint16_t version;                
    AxisCal  axes[AXIS_CAL_SLOTS];   
    uint16_t crc;                    
};

static_assert(sizeof(AxisCal) == 8,  "AxisCal layout changed — bump CAL_VERSION");
static_assert(sizeof(CalBlob) == 72, "CalBlob layout changed — bump CAL_VERSION");
static_assert(offsetof(CalBlob, crc) == 70, "CRC coverage is offsetof(crc); layout changed");

uint16_t calCrc16(const uint8_t* data, size_t len);

bool axisCalValid(const AxisCal& cal);

uint16_t axisCalApply(const AxisCal& cal, uint16_t raw);

uint16_t calBlobCrc(const CalBlob& blob);

bool calBlobValid(const CalBlob& blob);

void calBlobClear(CalBlob& blob);

void calBlobSeal(CalBlob& blob);

// ── Calibration wire protocol ─────────────────────────────────────────────────
//
// The USB CDC protocol between SimGateway and SkyHawkClient. Specified in
// FirmwarePlan/03-uart-usb-hid-protocol.md § Calibration Protocol (USB CDC), which is
// authoritative — if this header disagrees with that page, the page wins.
//
// Only the pure parts live here: the length table, the frame builder, and whole-frame
// validation. The byte-at-a-time receive state machine belongs to SimGateway.cpp because
// it has to re-emit rejected bytes into the relay.

constexpr uint8_t CAL_PROTO_VERSION = 1;

constexpr uint8_t CAL_FRAME_MAGIC[4] = { 0xAA, 0x53, 0x4B, 0x43 };   // "\xAA S K C"

constexpr uint16_t CAL_ENVELOPE_BYTES = 10;   
constexpr uint16_t CAL_MAX_PAYLOAD    = 82;   
constexpr uint16_t CAL_MAX_FRAME      = CAL_ENVELOPE_BYTES + CAL_MAX_PAYLOAD;  // 92
constexpr uint16_t CAL_MAX_INBOUND_PAYLOAD = 260;
constexpr uint16_t CAL_MAX_INBOUND_FRAME   = CAL_ENVELOPE_BYTES + CAL_MAX_INBOUND_PAYLOAD;  // 270

enum CalType : uint8_t {
    CAL_T_HELLO         = 0x01,
    CAL_T_GET_CAL       = 0x02,
    CAL_T_SESSION_OPEN  = 0x03,
    CAL_T_SESSION_CLOSE = 0x04,
    CAL_T_COMMIT        = 0x05,
    CAL_T_RESET         = 0x06,
    CAL_T_KEEPALIVE     = 0x07,
    CAL_T_STREAM_SELECT = 0x08,
    CAL_T_GET_DIAG      = 0x09,   // link-health counters (#355); answered at any time
    CAL_T_UPL_INFO      = 0x10,   // uploader state (#366); answered at any time
    CAL_T_UPL_BEGIN     = 0x11,   // open an upload session: image length + CRC
    CAL_T_UPL_DATA      = 0x12,   // 256 image bytes at an offset (the one >82-byte type, inbound only)
    CAL_T_UPL_END       = 0x13,   // verify the staged image
    CAL_T_UPL_FLASH     = 0x14,   // flash it into a board; the Gateway finishes on its own
    CAL_T_UPL_ABORT     = 0x15,   // drop the session and the staged image (not while flashing)

    CAL_T_HELLO_ACK     = 0x81,
    CAL_T_CAL_DATA      = 0x82,
    CAL_T_SESSION_ACK   = 0x83,
    CAL_T_ACK           = 0x84,
    CAL_T_NACK          = 0x85,
    CAL_T_RAW           = 0x86,
    CAL_T_DIAG          = 0x87,
    CAL_T_UPL_INFO_ACK  = 0x90,
    CAL_T_UPL_STATUS    = 0x91,   // a flash's progress, while the client keeps the session alive
    CAL_T_UPL_RESULT    = 0x92,   // a flash's outcome
};

enum CalNackReason : uint8_t {
    CAL_NACK_BAD_CRC      = 0x01,
    CAL_NACK_BAD_LENGTH   = 0x02,
    CAL_NACK_BAD_TYPE     = 0x03,
    CAL_NACK_BAD_INDEX    = 0x04,
    CAL_NACK_BAD_ORDER    = 0x05,
    CAL_NACK_NO_SESSION   = 0x06,
    CAL_NACK_NO_STORAGE   = 0x07,
    CAL_NACK_BAD_DEADZONE = 0x08,
    CAL_NACK_BUSY         = 0x09,   // another session (calibration / upload) or a flash is running
    CAL_NACK_TOO_LARGE    = 0x0A,   // UPL_BEGIN length over UPL_MAX_IMAGE_BYTES
    CAL_NACK_BAD_OFFSET   = 0x0B,   // UPL_DATA outside the announced length, or not chunk-aligned
    CAL_NACK_IMAGE_CRC    = 0x0C,   // UPL_END: the staged bytes don't match UPL_BEGIN's CRC
    CAL_NACK_BAD_IMAGE    = 0x0D,   // UPL_END: no valid identity block, or it disagrees with UPL_BEGIN
    CAL_NACK_NOT_STAGED   = 0x0E,   // UPL_FLASH without a verified staged image
};

constexpr uint8_t CAL_AXIS_NONE = 0xFF;

// ── Payload field offsets ─────────────────────────────────────────────────────
//
// Byte offsets into each type's payload (not the frame — the payload starts after the 8-byte
// header). Multi-byte fields are little-endian, matching the envelope's LEN. A field runs to the
// next offset; the last runs to the type's length in calLenValidForType(). Types with no payload
// have no enum. Named so the client sync generates them rather than hand-typing them (#346).

enum CalHelloAckField : uint8_t {
    CAL_HELLO_ACK_PROTO        = 0,  // u8: CAL_PROTO_VERSION
    CAL_HELLO_ACK_BLOB_VERSION = 1,  // u8: CAL_VERSION
    CAL_HELLO_ACK_AXIS_SLOTS   = 2,  // u8: AXIS_CAL_SLOTS
    CAL_HELLO_ACK_FW_MAJOR     = 3,  // u8: firmware release version (FirmwareVersion.h)
    CAL_HELLO_ACK_FW_MINOR     = 4,  // u8
    CAL_HELLO_ACK_FW_PATCH     = 5,  // u8
};

enum CalDataField : uint8_t {
    CAL_DATA_PRESENT_MASK    = 0,  // u8: bit i = an HIDAxis uses slot i
    CAL_DATA_CALIBRATED_MASK = 1,  // u8: bit i = slot i passes axisCalValid()
    CAL_DATA_RECORDS         = 2,  // AXIS_CAL_SLOTS records of CAL_DATA_REC_BYTES, slot order
};

constexpr uint8_t CAL_DATA_REC_BYTES = 10;

enum CalDataRecField : uint8_t {
    CAL_DATA_REC_CONTROL_ID = 0,  // u16: HID controlId of the axis in this slot, 0 = absent
    CAL_DATA_REC_MIN        = 2,  // u16: AxisCal::min
    CAL_DATA_REC_CENTRE     = 4,  // u16: AxisCal::centre
    CAL_DATA_REC_MAX        = 6,  // u16: AxisCal::max
    CAL_DATA_REC_DEADZONE   = 8,  // u16: AxisCal::deadzone, always 0
};

enum CalSessionOpenField : uint8_t {
    CAL_SESSION_OPEN_AXIS_IDX = 0,  // u8: axis to stream, CAL_AXIS_NONE = none
};

enum CalSessionAckField : uint8_t {
    CAL_SESSION_ACK_TIMEOUT_MS = 0,  // u32: session inactivity timeout, ms
    CAL_SESSION_ACK_AXIS_IDX   = 4,  // u8: echoed streamed-axis selection
};

enum CalStreamSelectField : uint8_t {
    CAL_STREAM_SELECT_AXIS_IDX = 0,  // u8: axis to stream, CAL_AXIS_NONE = none
};

enum CalCommitField : uint8_t {
    CAL_COMMIT_IDX      = 0,  // u8: axis slot
    CAL_COMMIT_MIN      = 1,  // u16
    CAL_COMMIT_CENTRE   = 3,  // u16
    CAL_COMMIT_MAX      = 5,  // u16
    CAL_COMMIT_DEADZONE = 7,  // u16: must be 0, else NACK BAD_DEADZONE
};

enum CalResetField : uint8_t {
    CAL_RESET_IDX = 0,  // u8: axis slot, CAL_AXIS_NONE = all
};

enum CalAckField : uint8_t {
    CAL_ACK_TYPE = 0,  // u8: CalType being acknowledged
};

enum CalNackField : uint8_t {
    CAL_NACK_TYPE   = 0,  // u8: CalType being refused
    CAL_NACK_REASON = 1,  // u8: CalNackReason
    CAL_NACK_DETAIL = 2,  // u8: offending axis slot, else CAL_AXIS_NONE
};

enum CalRawField : uint8_t {
    CAL_RAW_IDX = 0,  // u8: streamed axis slot
    CAL_RAW_RAW = 1,  // u16: reading before calibration
    CAL_RAW_CAL = 3,  // u16: the same reading through axisCalApply()
};

enum CalDiagField : uint8_t {
    CAL_DIAG_UPTIME_MS          = 0,   // u32: millis() when the reply was built
    CAL_DIAG_FIRST_UART_ERR_MS  = 4,   // u32: first PL011 error this boot
    CAL_DIAG_LAST_UART_ERR_MS   = 8,   // u32: latest PL011 error
    CAL_DIAG_FIRST_USB_LOSS_MS  = 12,  // u32: first healthy → unhealthy host-link edge
    CAL_DIAG_LAST_USB_LOSS_MS   = 16,  // u32: latest such edge
    CAL_DIAG_UART_OVERRUN       = 20,  // u16: RSR OE — the UART IRQ itself was held off
    CAL_DIAG_UART_FRAMING       = 22,  // u16: RSR FE — electrical
    CAL_DIAG_UART_BREAK         = 24,  // u16: RSR BE — RX held low (PanelBridge reset/off)
    CAL_DIAG_UART_PARITY        = 26,  // u16: RSR PE
    CAL_DIAG_UART_RX_OVERFLOW   = 28,  // u16: software RX queue full — loop() fell behind
    CAL_DIAG_LINES_DROPPED      = 30,  // u16: whole line dropped, no CDC room (host not reading)
    CAL_DIAG_LINES_CORRUPT      = 32,  // u16: line held a non-ASCII byte, not relayed
    CAL_DIAG_LINES_OVERLONG     = 34,  // u16: line exceeded the relay buffer, not relayed
    CAL_DIAG_FRAMES_DROPPED     = 36,  // u16: calibration frame dropped, no CDC room
    CAL_DIAG_UART_RESTARTS      = 38,  // u16: FAULT held too long → UART restarted
    CAL_DIAG_USB_REATTACHES     = 40,  // u16: recovery ladder detach/attach cycles
    CAL_DIAG_CDC_STALLS         = 42,  // u16: host held the port but the CDC IN endpoint stopped draining
    CAL_DIAG_RECOVERY_REBOOTS   = 44,  // u8:  recovery ladder reboots (survives reboot)
    CAL_DIAG_WATCHDOG_RESETS    = 45,  // u8:  hardware watchdog resets (survives reboot)
    CAL_DIAG_RESET_REASON       = 46,  // u8:  CalDiagResetReason for this boot
    CAL_DIAG_RESERVED           = 47,  // 17 bytes: reserved, transmit 0 (#295)
};

constexpr uint8_t CAL_DIAG_BYTES = 64;

enum CalDiagResetReason : uint8_t {
    CAL_DIAG_RESET_POWER_ON = 0,  // cold start, RUN pin, or anything not listed below
    CAL_DIAG_RESET_WATCHDOG = 1,  // hardware watchdog: loop() stopped running
    CAL_DIAG_RESET_RECOVERY = 2,  // the USB recovery ladder rebooted on purpose
};

static_assert(CAL_DIAG_BYTES <= CAL_MAX_PAYLOAD, "DIAG must fit the largest legal payload");

static_assert(CAL_DATA_RECORDS + AXIS_CAL_SLOTS * CAL_DATA_REC_BYTES == CAL_MAX_PAYLOAD,
              "CAL_DATA layout no longer fills CAL_MAX_PAYLOAD");

bool calLenValidForType(uint8_t type, uint16_t len);

uint16_t calBuildFrame(uint8_t* out, uint16_t outCap,
                       uint8_t type, uint8_t seq,
                       const uint8_t* payload, uint16_t len);

bool calFrameCrcOk(const uint8_t* frame, uint16_t n);

} // namespace OpenSkyhawk
```


