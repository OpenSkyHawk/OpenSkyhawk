# BootProtocol — Technical Specification

**Status:** Done
**FirmwarePlan ref:** `FirmwarePlan/12-bootloader.md#update-protocol`, `FirmwarePlan/02-can-protocol.md#frame-type-table`, `FirmwarePlan/00-decisions.md#d26`, `#d27`, `#d30`
**Depends on:** `<stdint.h>` only — header-only, no Arduino, no HAL

---

## Responsibility

The bootloader's wire contract (frozen at protocol 1, #367): frame IDs, command and reply codes, the announce layout,
`BootState`, `Status`, the transfer shape, the flash layout constants and the backup-register entry
flag and crash-loop counter, the bridge build's serial framing and enter request. The one source for every side of an update — the bootloader
(`Firmware/Bootloader`), STM32Board (`markBootHealthy()` clears the counter, #364), CANProtocol
(entering the bootloader, READY_n byte 3 — #365), the Gateway uploader (#366), the client sync
(SkyHawkClient#175/#177) and the bench host (`Firmware/Bootloader/tools/flash_can.py`, which parses it).

Does **not** own: the image identity block (`FirmwareImage`), the bootloader's behaviour
(`Bootloader`).

## File Layout

```
Firmware/Libraries/BootProtocol/
├── BootProtocol.h     header-only (library.json srcFilter -<*>, frameworks *)
└── library.json       version rewritten by release-please
```

## Public API

```cpp
namespace OpenSkyhawk::Boot {
constexpr uint8_t  PROTOCOL_VERSION = 1;          // READY_n byte 3 (#365); 0 = no bootloader
constexpr uint16_t CMD_ID_BASE = 0x740, RSP_ID_BASE = 0x780;
constexpr uint16_t canIdBootCmd(uint8_t n);       // 0x740 + n
constexpr uint16_t canIdBootRsp(uint8_t n);       // 0x780 + n
constexpr uint16_t UNCONFIGURED_RSP_ID = 0x7C1;   // announce with no NODE_ID
constexpr uint16_t UNCONFIGURED_CMD_ID = 0x7C0;   // CONNECT [uid16] binds the one unconfigured board

enum class Cmd : uint8_t   { CONNECT = 0x01, BEGIN = 0x02, BLOCK = 0x10, DATA_0 = 0x20, DATA_9 = 0x29, END = 0x30, GO = 0x40,
                             SET_NODE_ID = 0x50 };
// BLOCK_ACK = [index:u16, status, next:u16] — next = the first block not yet written (#366)
enum class Reply : uint8_t { CONNECTED = 0x81, BEGUN = 0x82, BLOCK_ACK = 0x90, ANNOUNCE = 0xA0, ENDED = 0xB0, GOING = 0xC0,
                             NODE_SET = 0xD0 };
enum class AnnounceField : uint8_t { CODE, PROTOCOL, ID, KIND, STATE, FLASH_KB, UID16 };   // byte offsets; UID16 is u16
constexpr uint8_t  ANNOUNCE_LEN = 8;
constexpr uint32_t UID_ADDRESS = 0x1FFFF7E8;
enum class BootState : uint8_t { REQUESTED = 0x01, APP_INVALID = 0x02, UNCONFIGURED = 0x03, APP_UNSTABLE = 0x04, CAUGHT = 0x05 };
enum class Status : uint8_t { OK, BAD_CRC, FLASH_ERROR, OUT_OF_RANGE, OUT_OF_ORDER, NO_APP, BAD_IMAGE, WRONG_NODE, WRONG_KIND };

constexpr uint16_t BLOCK_SIZE = 64;  constexpr uint8_t DATA_BYTES = 7, DATA_FRAMES = 10;
constexpr uint16_t ANNOUNCE_INTERVAL_MS = 1000;
constexpr uint32_t BOOTLOADER_SIZE = 0x1000, APP_OFFSET = BOOTLOADER_SIZE;
constexpr uint8_t  ENTRY_BKP_REGISTER = 1;   constexpr uint16_t ENTRY_MAGIC = 0xB007;
constexpr uint8_t  ATTEMPTS_BKP_REGISTER = 2; constexpr uint8_t ATTEMPT_LIMIT = 3;
constexpr uint16_t CATCH_WINDOW_MS = 200, IDLE_TIMEOUT_MS = 30000;

// #365
constexpr uint32_t INFO_ADDRESS = 0x08000FF0, INFO_MAGIC = 0x4C42534F;   // "OSBL"
struct BootloaderInfo { uint32_t magic; uint8_t protocolVersion; uint8_t transports; uint8_t reserved[10]; };   // 16 B
constexpr uint8_t  TRANSPORT_CAN = 0x01, TRANSPORT_SERIAL = 0x02;   // #367: `transports` bits; 0xFF (#365/#366) = both
constexpr uint16_t RELAY_EXPORT_ADDRESS = 0xFF10;  constexpr uint8_t RELAY_EXPORT_WORDS = 32;   // #366: a byte stream
constexpr uint8_t  RELAY_FRAME_MARK = 0xE0, RELAY_ESCAPE = 0x56, RELAY_ESC_55 = 0x01, RELAY_ESC_56 = 0x02;
constexpr uint8_t  WINDOW_BLOCKS = 4;   // blocks a host may send ahead of the last ack (#366)
#define BOOT_RSP_MSG_NAME "_BOOT_RSP"

// #367 — the bridge build: each frame on USART2 as 0xAA, id lo, id hi, len, data, sum (mod 256)
constexpr uint8_t  SERIAL_FRAME_START = 0xAA;
constexpr uint32_t SERIAL_BAUD = 250000;
constexpr uint16_t BRIDGE_ENTER_EXPORT_ADDRESS = 0xFF02;   // export write → PanelBridge resets into its bootloader
}
```

## Implementation Notes

- **Named values only** (#346): every wire value is `enum class X : T { NAME = literal }` or
  `constexpr T NAME = literal`, and every name is unique across the header — the bench host maps names
  to values with one regex, and the client sync reads the same forms.
- **Bump `PROTOCOL_VERSION`** on any change to a frame's layout or meaning. New commands, replies,
  states and statuses are appends.
- `APP_OFFSET` must equal `board_build.flash_offset` in `Firmware/shared/stm32.ini`.
- **No `NAME = value` text in comments.** The bench host's regex — and the client sync — read that form;
  a comment like "RELAY_ESC_55 = 0x55" overwrote the real value once (#366). Say "decodes to 0x55".
- **No field may be called `NODE_ID`** — every node build defines that macro (the announce field is
  `ID`, as in #346's row fields).
- `PROTOCOL_VERSION` stayed 1 through #364: no board shipped #363's 6-byte announce.

## Dependencies

None beyond `<stdint.h>`.
