

# File BootProtocol.h

[**File List**](files.md) **>** [**BootProtocol**](dir_363c1f8b3b61005f0653a81dde367b4b.md) **>** [**BootProtocol.h**](BootProtocol_8h.md)

[Go to the documentation of this file](BootProtocol_8h.md)


```C++
#pragma once
#include <stdint.h>

namespace OpenSkyhawk {
namespace Boot {

constexpr uint8_t PROTOCOL_VERSION = 1;

// ── CAN frame IDs (11-bit) ────────────────────────────────────────────────────
// The lowest priority on the bus, so update traffic never delays a control.

constexpr uint16_t CMD_ID_BASE = 0x740;  
constexpr uint16_t RSP_ID_BASE = 0x780;  

constexpr uint16_t canIdBootCmd(uint8_t n) { return CMD_ID_BASE + n; }
constexpr uint16_t canIdBootRsp(uint8_t n) { return RSP_ID_BASE + n; }

constexpr uint16_t UNCONFIGURED_RSP_ID = 0x7C1;
constexpr uint16_t UNCONFIGURED_CMD_ID = 0x7C0;

// ── Host → bootloader (BOOT_CMD_n), byte 0 ───────────────────────────────────

enum class Cmd : uint8_t {
    CONNECT = 0x01,  
    BEGIN   = 0x02,  
    BLOCK   = 0x10,  
    DATA_0  = 0x20,  
    DATA_9  = 0x29,  
    END     = 0x30,  
    GO      = 0x40,  
};

// ── Bootloader → host (BOOT_RSP_n), byte 0 ───────────────────────────────────

enum class Reply : uint8_t {
    CONNECTED   = 0x81,  
    BEGUN       = 0x82,  
    BLOCK_ACK   = 0x90,  
    ANNOUNCE    = 0xA0,  
    ENDED       = 0xB0,  
    GOING       = 0xC0,  
};

enum class AnnounceField : uint8_t {
    CODE     = 0,  
    PROTOCOL = 1,  
    NODE_ID  = 2,  
    KIND     = 3,  
    STATE    = 4,  
    FLASH_KB = 5,  
};
constexpr uint8_t ANNOUNCE_LEN = 6;

enum class BootState : uint8_t {
    REQUESTED    = 0x01,  
    APP_INVALID  = 0x02,  
    UNCONFIGURED = 0x03,  
    // 0x04-0xFF reserved (#364: app unstable, caught at boot)
};

enum class Status : uint8_t {
    OK           = 0x00,
    BAD_CRC      = 0x01,  
    FLASH_ERROR  = 0x02,  
    OUT_OF_RANGE = 0x03,  
    OUT_OF_ORDER = 0x04,  
    NO_APP       = 0x05,  
    BAD_IMAGE    = 0x06,  
};

// ── Transfer shape ────────────────────────────────────────────────────────────

constexpr uint16_t BLOCK_SIZE        = 64;  
constexpr uint8_t  DATA_BYTES        = 7;   
constexpr uint8_t  DATA_FRAMES       = 10;  
constexpr uint16_t ANNOUNCE_INTERVAL_MS = 1000;
// Block CRC: CRC-16/CCITT-FALSE (poly 0x1021, init 0xFFFF) over the 64 bytes — the same CRC the
// Gateway's CDC frames use. Image CRC: the identity block's CRC-32/MPEG-2 (FirmwareImage.h).

// ── Layout and entry ──────────────────────────────────────────────────────────

constexpr uint32_t BOOTLOADER_SIZE = 0x1000;  
constexpr uint32_t APP_OFFSET      = BOOTLOADER_SIZE;

constexpr uint8_t  ENTRY_BKP_REGISTER = 1;
constexpr uint16_t ENTRY_MAGIC        = 0xB007;

}  // namespace Boot
}  // namespace OpenSkyhawk
```


