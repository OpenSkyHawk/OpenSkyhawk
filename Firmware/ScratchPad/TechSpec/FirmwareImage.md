# FirmwareImage — Technical Specification

**Status:** Done
**FirmwarePlan ref:** `FirmwarePlan/08-hardware-firmware-contracts.md#image-layout--identity-block-362`, `FirmwarePlan/00-decisions.md#d25`
**Depends on:** `FirmwareVersion` (the version fields), `NodeStatus` (`NodeKind`) — header-only

---

## Responsibility

Owns the **image identity block**: the 32-byte struct every production STM32 image carries at image
start + `0x200`, its constants, its field offsets, and the CRC algorithm's test vectors. Readers on
the far side — the bootloader (#363), the Gateway uploader (#366) and the client
(SkyHawkClient#177) — must match this header.

Does **not** own: placing the block (the shared linker script), stamping the length and CRC (the
post-build step), or checking an image (the bootloader and uploader).

## File Layout

```
Firmware/Libraries/FirmwareImage/
├── FirmwareImage.h     header-only (library.json srcFilter -<*>)
└── library.json        version rewritten by release-please
Firmware/shared/
├── stm32f103.ld        STM32 core 4.30000.0 F103C8/CB script + .osh_identity at +0x200, 0xFF fill
├── stm32.ini           [env] ldscript + post script + the bootloader offset (#363)
├── osh_image.py        post-build: stamp imageLength/imageCrc into the .elf, write the .bin 0xFF-filled
└── tests/test_osh_image.py   CRC vectors + offsets read from FirmwareImage.h; runs in Firmware CI
```

## Public API

```cpp
namespace OpenSkyhawk::FirmwareImage {
constexpr uint32_t IDENTITY_OFFSET = 0x200;        // from the image's start
constexpr uint32_t MAGIC           = 0x594B534Fu;  // "OSKY" in memory order
constexpr uint16_t LAYOUT_VERSION  = 1;
constexpr uint32_t UNSTAMPED       = 0xFFFFFFFFu;  // imageLength / imageCrc before the stamp

struct Identity { /* 32 bytes — see Key Data Structures */ };

constexpr uint32_t OFF_MAGIC, OFF_LAYOUT, OFF_SIZE, OFF_LENGTH, OFF_CRC,
                   OFF_NODE_ID, OFF_NODE_KIND, OFF_FW_MAJOR, OFF_FW_MINOR, OFF_FW_PATCH;

constexpr uint32_t CRC_VECTOR_WORD_12345678 = 0xDF8A8A2Bu;  // one word 0x12345678
constexpr uint32_t CRC_VECTOR_BYTES_00_0F   = 0x081B46CAu;  // bytes 0x00..0x0F
}

// Define this image's identity block — exactly once per image, in the role library.
#define OSH_FIRMWARE_IDENTITY(kind)
```

Used by `PanelGroup.cpp` (`NodeKind::PANEL_GROUP`), `PduNode.cpp` (`NodeKind::PDU`) and
`PanelBridge.cpp` (`NodeKind::BRIDGE`).

## Key Data Structures

| Offset | Type | Field | Notes |
|---|---|---|---|
| 0 | `uint32_t` | `magic` | `MAGIC` |
| 4 | `uint16_t` | `layoutVersion` | `LAYOUT_VERSION` |
| 6 | `uint16_t` | `blockSize` | `sizeof(Identity)`; readers parse the prefix they know |
| 8 | `uint32_t` | `imageLength` | stamped; bytes from the image's start, padded to 4 |
| 12 | `uint32_t` | `imageCrc` | stamped; CRC-32/MPEG-2 over `imageLength` bytes, this word skipped |
| 16 | `uint8_t` | `nodeId` | `NODE_ID` |
| 17 | `uint8_t` | `nodeKind` | `NodeKind` |
| 18 | `uint8_t` ×3 | `fwMajor/Minor/Patch` | `OPENSKYHAWK_FW_*` |
| 21 | `uint8_t[11]` | `reserved` | 0 |

`static_assert(sizeof(Identity) == 32)`.

## Implementation Notes

- **Section + KEEP.** The macro puts the struct in `.osh_identity` with `used`; the shared linker
  script `KEEP`s it at `.isr_vector` start + `0x200` with `FILL(0xFF)` before it. `Tests/` builds use
  the core's default script, which has no `KEEP` for the section, so `--gc-sections` drops it there.
- **Stamp order.** `osh_image.py` flattens the `.elf` with `objcopy --gap-fill 0xFF`, pads to a word,
  writes `imageLength` into the buffer, then computes the CRC with the `imageCrc` word skipped, and
  patches both words into the `.elf`'s `.isr_vector` section in place. The `.bin` is then rebuilt
  from the stamped `.elf` with the same 0xFF fill.
- **No block, no stamp.** An image without the magic at `0x200` builds normally; the step prints
  "no identity block in this image — not stamped".
- **Bootloader offset.** The script takes the app's start from `LD_FLASH_OFFSET`
  (`board_build.flash_offset`), so the block sits at `0x08001200` behind the 4 KB bootloader (#363).

## Dependencies

`FirmwareVersion`, `NodeStatus`; Python 3 standard library and the toolchain's `objcopy` for the
post-build step.
