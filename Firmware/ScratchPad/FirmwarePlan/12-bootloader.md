# 12 — CAN Bootloader

**Owns:** the bootloader every STM32 board carries — flash layout, boot flow, the NODE_ID / kind
copy in the option bytes, and the update protocol on `BOOT_CMD_n` / `BOOT_RSP_n`.
**Does not own:** the CAN ID map (`02-can-protocol.md` — the IDs are listed there), the image
identity block (`08-hardware-firmware-contracts.md#image-layout--identity-block-362`), the app side
of entering the bootloader and the bridge relay (#365), the Gateway uploader (#366).

Code: `Firmware/Bootloader/` (the bootloader), `Firmware/Libraries/BootProtocol/BootProtocol.h` (every
wire value below — the one source the bootloader, CANProtocol, the uploader and the client sync
read). Rationale: D26. Safety features — write protection, the NODE_ID rule, set-node-ID, the first
image on an unconfigured board, the crash-loop counter, catch-at-boot, the idle timeout — are #364.

---

## Flash Layout

| Region | Address | Size | Owner |
|---|---|---|---|
| Bootloader | `0x08000000` | 4 KB (one write-protect block) | `Firmware/Bootloader` — replaced only over ST-Link |
| App | `0x08001000` | flash − 4 KB (60 KB C8, 124 KB CB) | every production project (`Firmware/shared/stm32.ini`: `board_build.flash_offset = 0x1000`) |
| App identity block | `0x08001200` | 32 B | inside the app (`08`) |
| PDU calibration page | `0x0800FC00` | 1 KB | PDU builds stop short of it (`board_upload.maximum_size = 64512`); the bootloader erases only the pages an image covers |

The core sets `VTOR` from the offset, so apps need no code change. An ST-Link upload (`pio run -t
upload`) writes the app only and keeps the bootloader. One bootloader image serves the C8 and the CB:
it reads the flash size from `0x1FFFF7E0`.

**Blank boards.** A board without the bootloader has nothing at `0x08000000` to start the app, so its
first flash carries both: `pio run -t bootstrap` (dev), or the release's `<board>-<v>-full.hex`. The
app's NODE_ID comes with it — a board gets its ID from its first app (D10 unchanged).

---

## Boot Flow

At every reset:

1. **Entry flag** — read backup register `DR1`; `ENTRY_MAGIC` (`0xB007`) means the app asked for the
   bootloader. Cleared on read. The backup domain survives a reset, not a power cycle.
2. **Clocks** — HSE 8 MHz × 9 = 72 MHz, APB1 36 MHz (STM32Board's setup). If the crystal doesn't
   start: HSI / 2 × 9 = 36 MHz with APB1 undivided — still 36 MHz for CAN, so the bit timing holds
   within HSI's ±1 % and the board stays reachable.
3. **App check** — the identity block's magic, layout and length are plausible, the vector table's
   stack pointer is in RAM and its reset vector inside the image, and the hardware CRC over
   `imageLength` (the `imageCrc` word skipped) matches `imageCrc`.
4. **NODE_ID / kind copy** — a valid app whose `nodeId` / `nodeKind` differ from option bytes
   Data0 / Data1: rewrite them (below) and reset.
5. **Jump** — valid app and no entry flag: SysTick off, CAN reset, clocks back to the reset state
   (HSI, PLL and HSE off, zero wait states), NVIC cleared, `VTOR` = app; then the stack switch and the
   branch in **one** inline-asm block (a compiler `pop` between them read the app's stack and
   bus-faulted on the #361 bench).
6. **Stay** otherwise, with `BootState`:

| BootState | When | Announces on |
|---|---|---|
| `REQUESTED` | entry flag, app valid | `BOOT_RSP_n`, n = Data0 |
| `APP_INVALID` | no valid app, Data0 set | `BOOT_RSP_n`, n = Data0 |
| `UNCONFIGURED` | no valid app, Data0 = `0xFF` | `UNCONFIGURED_RSP_ID` (`0x7C1`); takes no commands until #364 |

Status LED while waiting: red slow blink (≈1 Hz); fast while an image arrives; green also on when
unconfigured.

### The NODE_ID / kind copy (option bytes Data0 / Data1)

The build stays the source of NODE_ID (D10); the option bytes hold **the ID of the last valid app**,
so a board whose app breaks still answers as its node. Written by read-modify-write: read all eight
option bytes, erase, program them back with Data0 / Data1 changed — RDP first. Written only when they
differ (a new board, or an ST-Link repurpose to another node), verified before the reset; a failed
write leaves them as they were and the app still starts.

> **The one dangerous window:** erasing the option bytes also erases RDP, and an erased RDP reads as
> read protection. A power cut between the erase and the RDP write (a few ms, once per new board or
> repurpose) leaves the chip protected — recover over ST-Link (`stm32f1x unlock 0`, which mass-erases),
> then reflash. #364's self write-protection uses the same routine and the same window.

---

## Update Protocol

All frames standard 11-bit, lowest bus priority. Multi-byte fields little-endian. Byte 0 is the
command (host → bootloader, `BOOT_CMD_n` `0x740+n`) or the reply (bootloader → host, `BOOT_RSP_n`
`0x780+n`).

| Host → bootloader | Bytes 1–7 | Reply |
|---|---|---|
| `CONNECT` `0x01` | — | `CONNECTED` `0x81` + the announce fields |
| `BEGIN` `0x02` | `len:u32` | `BEGUN` `0x82` `[status]` |
| `BLOCK` `0x10` | `index:u16, crc16:u16` | — (the 10 DATA frames follow) |
| `DATA_k` `0x20+k` (k = 0–9) | 7 bytes of the block, bytes 7k…7k+6 | after `DATA_9`: `BLOCK_ACK` `0x90` `[index:u16, status]` |
| `END` `0x30` | `crc:u32` (the identity block's `imageCrc`) | `ENDED` `0xB0` `[status, crc:u32 computed]` |
| `GO` `0x40` | — | `GOING` `0xC0` `[status]`, then the board resets into the app |

**Announce** (`ANNOUNCE` `0xA0`, every 1 s while no image is arriving; `CONNECTED` carries the same):
`[code, PROTOCOL_VERSION, NODE_ID (Data0), kind (Data1), BootState, flash KB]`, 6 bytes. Capacity =
flash KB × 1024 − 4 KB.

**Status:** `OK` 0 · `BAD_CRC` 1 · `FLASH_ERROR` 2 · `OUT_OF_RANGE` 3 · `OUT_OF_ORDER` 4 · `NO_APP` 5 ·
`BAD_IMAGE` 6.

### Transfer rules

- **BEGIN** checks `0 < len` and that the image, padded to whole blocks, fits; then **erases the app's
  first page — the old app is invalid from here on.**
- **Blocks** are 64 bytes, sent in order from 0; the image is padded with `0xFF`. CRC-16/CCITT-FALSE
  over the 64 bytes (the CDC frames' CRC). A page is erased when its first block arrives — pages past
  the image are never touched.
- **A resent block** (the host missed the ack) equal to the last one written is acked `OK` without
  rewriting. Any other index out of order → `OUT_OF_ORDER`.
- **Block 0** (the vector table) is held in RAM, not written.
- **END** needs every block. It checks the identity block (in flash, block 8) — magic, `imageLength`
  ≤ the transfer, `imageCrc` = END's CRC, a plausible vector table — then computes the CRC over
  `imageLength` with block 0 from RAM. On a match it **writes block 0 last (commit)**: only now does the
  app pass the boot check. An interrupted transfer — at any point — leaves "in bootloader, flashable".
- **GO** with a valid app replies `OK` and resets; the normal boot flow starts the app.
- Images without an identity block are refused (`BAD_IMAGE`) — a sketch needs a role library
  (PanelGroup, PowerMonitor, PanelBridge) to have one.

Until #364, an upload may carry a different NODE_ID: the next boot copies it into Data0. #364's NODE_ID
rule refuses that.

---

## Bench Host

`Firmware/Bootloader/tools/flash_can.py` drives the protocol through a CANable (slcan, python-can)
before the Gateway uploader exists. It reads the frame IDs and codes from `BootProtocol.h`, takes the
length and CRC from the image's identity block, and can stop mid-image (`--stop-after N`) for fault
tests. Without #365, request the bootloader over ST-Link: write `0xB007` to `DR1` (`0x40006C04`, with
the PWR / BKP clocks and `PWR_CR.DBP` on) and reset.
