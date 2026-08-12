

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

enum CalType : uint8_t {
    CAL_T_HELLO         = 0x01,
    CAL_T_GET_CAL       = 0x02,
    CAL_T_SESSION_OPEN  = 0x03,
    CAL_T_SESSION_CLOSE = 0x04,
    CAL_T_COMMIT        = 0x05,
    CAL_T_RESET         = 0x06,
    CAL_T_KEEPALIVE     = 0x07,
    CAL_T_STREAM_SELECT = 0x08,

    CAL_T_HELLO_ACK     = 0x81,
    CAL_T_CAL_DATA      = 0x82,
    CAL_T_SESSION_ACK   = 0x83,
    CAL_T_ACK           = 0x84,
    CAL_T_NACK          = 0x85,
    CAL_T_RAW           = 0x86,
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
};

constexpr uint8_t CAL_AXIS_NONE = 0xFF;

bool calLenValidForType(uint8_t type, uint16_t len);

uint16_t calBuildFrame(uint8_t* out, uint16_t outCap,
                       uint8_t type, uint8_t seq,
                       const uint8_t* payload, uint16_t len);

bool calFrameCrcOk(const uint8_t* frame, uint16_t n);

} // namespace OpenSkyhawk
```


