

# File UploadProtocol.h

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**SimGateway**](dir_a54aa0246e1c520ae49dfef506a428ca.md) **>** [**UploadProtocol.h**](UploadProtocol_8h.md)

[Go to the documentation of this file](UploadProtocol_8h.md)


```C++

#pragma once

#include <stdint.h>

namespace OpenSkyhawk {

constexpr uint8_t  UPL_PROTO_VERSION   = 1;
constexpr uint32_t UPL_MAX_IMAGE_BYTES = 124u * 1024u;   
constexpr uint16_t UPL_CHUNK_BYTES     = 256;            

constexpr uint8_t UPL_TARGET_BRIDGE       = 0x00;   
constexpr uint8_t UPL_TARGET_UNCONFIGURED = 0xFE;   
constexpr uint8_t UPL_NODE_ID_KEEP        = 0xFF;   

enum UplState : uint8_t {
    UPL_STATE_IDLE     = 0,   // no session
    UPL_STATE_STAGING  = 1,   // UPL_BEGIN seen, receiving UPL_DATA
    UPL_STATE_STAGED   = 2,   // UPL_END verified the image; ready for UPL_FLASH
    UPL_STATE_FLASHING = 3,   // a flash is running — the Gateway owns the UART until it ends
};

enum UplPhase : uint8_t {
    UPL_PHASE_ENTERING_BOOT = 1,   // asking the board for its bootloader (CONNECT)
    UPL_PHASE_CONNECTED     = 2,   // its bootloader answered; checking ID, kind and size
    UPL_PHASE_SETTING_ID    = 3,   // SET_NODE_ID to UPL_FLASH's newNodeId
    UPL_PHASE_ERASING       = 4,   // BEGIN: the bootloader erases the pages the image needs
    UPL_PHASE_WRITING       = 5,   // blocks `block` of `blocks`
    UPL_PHASE_VERIFYING     = 6,   // END: the bootloader checks the CRC and commits
    UPL_PHASE_REBOOTING     = 7,   // GO
    UPL_PHASE_CONFIRMING    = 8,   // waiting for the app to report the image's version
};

enum UplResult : uint8_t {
    UPL_RESULT_NONE              = 0x00,   // no flash yet (UPL_INFO_ACK only)
    UPL_RESULT_OK                = 0x01,   // the app runs and reports the image's version
    UPL_RESULT_NO_RESPONSE       = 0x02,   // nothing answered at that NODE_ID
    UPL_RESULT_NO_BOOTLOADER     = 0x03,   // the board is there (_NODE_STATUS) but has no bootloader
    UPL_RESULT_ID_MISMATCH       = 0x04,   // the image is for another NODE_ID and no newNodeId was given
    UPL_RESULT_KIND_MISMATCH     = 0x05,   // the image is for another kind of board — never flashed
    UPL_RESULT_TOO_BIG_FOR_BOARD = 0x06,   // the image doesn't fit the board's flash
    UPL_RESULT_WRITE_FAILED      = 0x07,   // erase / write refused, or the transfer stalled, after retries
    UPL_RESULT_VERIFY_FAILED     = 0x08,   // the bootloader's CRC check failed, after retries
    UPL_RESULT_VERSION_MISMATCH  = 0x09,   // the board rebooted into a different version
    UPL_RESULT_TIMEOUT           = 0x0A,   // the board never came back after GO
    UPL_RESULT_UNSUPPORTED       = 0x0B,   // not sent since #367 (kept so the code stays taken): target 0 with nothing to flash is NACKed NOT_STAGED
};

constexpr uint8_t UPL_RETRIES = 2;   

// ── Payload field offsets (little-endian) ─────────────────────────────────────

enum UplInfoAckField : uint8_t {
    UPL_INFO_ACK_PROTO        = 0,   // u8: UPL_PROTO_VERSION
    UPL_INFO_ACK_MAX_IMAGE_KB = 1,   // u8: UPL_MAX_IMAGE_BYTES / 1024
    UPL_INFO_ACK_CHUNK        = 2,   // u16: UPL_CHUNK_BYTES
    UPL_INFO_ACK_STATE        = 4,   // u8: UplState
    UPL_INFO_ACK_LAST_TARGET  = 5,   // u8: the last flash's target
    UPL_INFO_ACK_LAST_RESULT  = 6,   // u8: UplResult (kept for a client that went away mid-flash)
    UPL_INFO_ACK_LAST_MAJOR   = 7,   // u8: the version it confirmed (0.0.0 = none)
    UPL_INFO_ACK_LAST_MINOR   = 8,
    UPL_INFO_ACK_LAST_PATCH   = 9,
    UPL_INFO_ACK_BRIDGE_MAJOR = 10,  // u8: the bridge image bundled in this Gateway firmware (#367; 0.0.0 = none)
    UPL_INFO_ACK_BRIDGE_MINOR = 11,
    UPL_INFO_ACK_BRIDGE_PATCH = 12,
    UPL_INFO_ACK_RESERVED     = 13,  // 3 bytes, 0
};

enum UplBeginField : uint8_t {
    UPL_BEGIN_LENGTH = 0,   // u32: the image's identity-block imageLength
    UPL_BEGIN_CRC    = 4,   // u32: its imageCrc (CRC-32/MPEG-2, the CRC word skipped — FirmwareImage.h)
};

enum UplDataField : uint8_t {
    UPL_DATA_OFFSET = 0,   // u32: a multiple of UPL_CHUNK_BYTES below the length
    UPL_DATA_BYTES  = 4,   // UPL_CHUNK_BYTES; the last chunk 0xFF-padded
};

enum UplFlashField : uint8_t {
    UPL_FLASH_TARGET      = 0,   // u8: NODE_ID 1-63, UPL_TARGET_UNCONFIGURED or UPL_TARGET_BRIDGE
    UPL_FLASH_NEW_NODE_ID = 1,   // u8: SET_NODE_ID first (must equal the image's nodeId), or UPL_NODE_ID_KEEP
};

enum UplStatusField : uint8_t {
    UPL_STATUS_PHASE    = 0,   // u8: UplPhase
    UPL_STATUS_TARGET   = 1,   // u8: the NODE_ID being flashed (the new one after SET_NODE_ID)
    UPL_STATUS_ATTEMPT  = 2,   // u8: 0 = first try, then 1..UPL_RETRIES
    UPL_STATUS_BLOCK    = 3,   // u16: blocks the bootloader has acknowledged
    UPL_STATUS_BLOCKS   = 5,   // u16: blocks in the image
    UPL_STATUS_RESERVED = 7,   // u8, 0
};

enum UplResultField : uint8_t {
    UPL_RESULT_RESULT   = 0,   // u8: UplResult
    UPL_RESULT_TARGET   = 1,   // u8: the NODE_ID flashed — for an unconfigured board, the one it adopted (OK)
    UPL_RESULT_MAJOR    = 2,   // u8: the version the board reported (0.0.0 = none)
    UPL_RESULT_MINOR    = 3,
    UPL_RESULT_PATCH    = 4,
    UPL_RESULT_ATTEMPTS = 5,   // u8: tries used (1 = first try)
    UPL_RESULT_SECONDS  = 6,   // u16: UPL_FLASH to result
};

}  // namespace OpenSkyhawk
```


