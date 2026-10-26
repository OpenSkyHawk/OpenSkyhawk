

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
    SET_NODE_ID = 0x50,  
};

// ── Bootloader → host (BOOT_RSP_n), byte 0 ───────────────────────────────────

enum class Reply : uint8_t {
    CONNECTED   = 0x81,  
    BEGUN       = 0x82,  
    BLOCK_ACK   = 0x90,  
    ANNOUNCE    = 0xA0,  
    ENDED       = 0xB0,  
    GOING       = 0xC0,  
    NODE_SET    = 0xD0,  
};

enum class AnnounceField : uint8_t {
    CODE     = 0,  
    PROTOCOL = 1,  
    ID       = 2,  
    KIND     = 3,  
    STATE    = 4,  
    FLASH_KB = 5,  
    UID16    = 6,  
};
constexpr uint8_t  ANNOUNCE_LEN = 8;
constexpr uint32_t UID_ADDRESS  = 0x1FFFF7E8;  

enum class BootState : uint8_t {
    REQUESTED    = 0x01,  
    APP_INVALID  = 0x02,  
    UNCONFIGURED = 0x03,  
    APP_UNSTABLE = 0x04,  
    CAUGHT       = 0x05,  
    // 0x06-0xFF reserved
};

enum class Status : uint8_t {
    OK           = 0x00,
    BAD_CRC      = 0x01,  
    FLASH_ERROR  = 0x02,  
    OUT_OF_RANGE = 0x03,  
    OUT_OF_ORDER = 0x04,  
    NO_APP       = 0x05,  
    BAD_IMAGE    = 0x06,  
    WRONG_NODE   = 0x07,  
    WRONG_KIND   = 0x08,  
};

// ── Transfer shape ────────────────────────────────────────────────────────────

constexpr uint16_t BLOCK_SIZE        = 64;  
constexpr uint8_t  WINDOW_BLOCKS     = 4;
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

// ── Bootloader info block (#365) ──────────────────────────────────────────────

constexpr uint32_t INFO_ADDRESS = 0x08000FF0;
constexpr uint32_t INFO_MAGIC   = 0x4C42534Fu;   

constexpr uint8_t TRANSPORT_CAN    = 0x01;   
constexpr uint8_t TRANSPORT_SERIAL = 0x02;   

struct BootloaderInfo {
    uint32_t magic;            
    uint8_t  protocolVersion;  
    uint8_t  transports;       
    uint8_t  reserved[10];     
};
static_assert(sizeof(BootloaderInfo) == 16, "the bootloader info block is 16 bytes");

// ── Relay through the Gateway (#365) ──────────────────────────────────────────
// The Gateway → PanelBridge UART carries only the DCS-BIOS export stream, so a host sends boot
// commands the way it asks for the roster: by writing reserved export addresses. Each export record at
// RELAY_EXPORT_ADDRESS (up to RELAY_EXPORT_WORDS words, low byte of each word first) carries a byte
// stream of whole CAN frames (#366):
//   RELAY_FRAME_MARK | len   (len 0-8), id lo, id hi, len data bytes
// with every 0x55 and 0x56 inside a frame sent as RELAY_ESCAPE + RELAY_ESC_55 / RELAY_ESC_56 — the
// DCS-BIOS parser resyncs on any four 0x55 bytes in a row, which image data could contain. Bytes
// between frames (padding) are 0x00 and ignored; every record starts on a frame boundary. PanelBridge
// puts each frame on CAN — only BOOT_CMD_n for n = 1-63 and UNCONFIGURED_CMD_ID — and never
// broadcasts the window as CTRL_BCAST. Every BOOT_RSP_n / UNCONFIGURED_RSP_ID frame goes the other way
// as one DCS-BIOS message: `_BOOT_RSP <id(3 hex)><length(1 hex)><data(2 hex per byte)>`.
constexpr uint16_t RELAY_EXPORT_ADDRESS = 0xFF10;
constexpr uint8_t  RELAY_EXPORT_WORDS   = 32;     
constexpr uint8_t  RELAY_FRAME_MARK     = 0xE0;   
constexpr uint8_t  RELAY_ESCAPE         = 0x56;
constexpr uint8_t  RELAY_ESC_55         = 0x01;   
constexpr uint8_t  RELAY_ESC_56         = 0x02;   
#define BOOT_RSP_MSG_NAME "_BOOT_RSP"

// ── The bridge's serial transport (#367) ──────────────────────────────────────
// The Gateway's PanelBridge is the CAN master: nothing on the bus can update it. Its bootloader is the
// `bridge` build of the same source (Firmware/Bootloader), which takes the same commands and answers
// with the same replies, as frames on USART2 (PA2 / PA3, the link to the Gateway's RP2040) instead of
// CAN — both transports don't fit 4 KB. The id is the CAN id the frame would have had (BOOT_CMD_0 /
// BOOT_RSP_0). One frame on the wire:
//   SERIAL_FRAME_START, id lo, id hi, len (0-8), data, sum   (sum = id lo + id hi + len + data, mod 256)
constexpr uint8_t  SERIAL_FRAME_START = 0xAA;
constexpr uint32_t SERIAL_BAUD        = 250000;   
constexpr uint16_t BRIDGE_ENTER_EXPORT_ADDRESS = 0xFF02;

constexpr uint8_t ATTEMPTS_BKP_REGISTER = 2;
constexpr uint8_t ATTEMPT_LIMIT         = 3;

constexpr uint16_t CATCH_WINDOW_MS  = 200;    
constexpr uint16_t IDLE_TIMEOUT_MS  = 30000;  

}  // namespace Boot
}  // namespace OpenSkyhawk
```


