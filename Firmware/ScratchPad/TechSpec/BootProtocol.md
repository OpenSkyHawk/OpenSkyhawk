# BootProtocol — Technical Specification

**Status:** Done
**FirmwarePlan ref:** `FirmwarePlan/12-bootloader.md#update-protocol`, `FirmwarePlan/02-can-protocol.md#frame-type-table`, `FirmwarePlan/00-decisions.md#d26`
**Depends on:** `<stdint.h>` only — header-only, no Arduino, no HAL

---

## Responsibility

The CAN bootloader's wire contract: frame IDs, command and reply codes, the announce layout,
`BootState`, `Status`, the transfer shape, the flash layout constants and the backup-register entry
flag. The one source for every side of an update — the bootloader (`Firmware/Bootloader`), CANProtocol
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
constexpr uint16_t UNCONFIGURED_CMD_ID = 0x7C0;   // reserved for #364

enum class Cmd : uint8_t   { CONNECT = 0x01, BEGIN = 0x02, BLOCK = 0x10, DATA_0 = 0x20, DATA_9 = 0x29, END = 0x30, GO = 0x40 };
enum class Reply : uint8_t { CONNECTED = 0x81, BEGUN = 0x82, BLOCK_ACK = 0x90, ANNOUNCE = 0xA0, ENDED = 0xB0, GOING = 0xC0 };
enum class AnnounceField : uint8_t { CODE, PROTOCOL, NODE_ID, KIND, STATE, FLASH_KB };   // byte offsets
constexpr uint8_t ANNOUNCE_LEN = 6;
enum class BootState : uint8_t { REQUESTED = 0x01, APP_INVALID = 0x02, UNCONFIGURED = 0x03 };
enum class Status : uint8_t { OK, BAD_CRC, FLASH_ERROR, OUT_OF_RANGE, OUT_OF_ORDER, NO_APP, BAD_IMAGE };

constexpr uint16_t BLOCK_SIZE = 64;  constexpr uint8_t DATA_BYTES = 7, DATA_FRAMES = 10;
constexpr uint16_t ANNOUNCE_INTERVAL_MS = 1000;
constexpr uint32_t BOOTLOADER_SIZE = 0x1000, APP_OFFSET = BOOTLOADER_SIZE;
constexpr uint8_t  ENTRY_BKP_REGISTER = 1;   constexpr uint16_t ENTRY_MAGIC = 0xB007;
}
```

## Implementation Notes

- **Named values only** (#346): every wire value is `enum class X : T { NAME = literal }` or
  `constexpr T NAME = literal`, and every name is unique across the header — the bench host maps names
  to values with one regex, and the client sync reads the same forms.
- **Bump `PROTOCOL_VERSION`** on any change to a frame's layout or meaning. New commands, replies,
  states and statuses are appends.
- `APP_OFFSET` must equal `board_build.flash_offset` in `Firmware/shared/stm32.ini`.

## Dependencies

None beyond `<stdint.h>`.
