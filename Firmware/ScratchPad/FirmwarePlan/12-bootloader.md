# 12 — Bootloader

**Owns:** the bootloader every STM32 board carries — its two builds (CAN for the nodes, USART2 for the
Gateway's PanelBridge), flash layout, boot flow, its self-protection, the NODE_ID / kind copy in the
option bytes, the crash-loop counter, the update protocol on `BOOT_CMD_n` / `BOOT_RSP_n` (and the
unconfigured pair, and the bridge's serial framing), and the freeze.
**Does not own:** the CAN ID map (`02-can-protocol.md` — the IDs are listed there), the image
identity block (`08-hardware-firmware-contracts.md#image-layout--identity-block-362`), the app side
of entering the bootloader (`STM32Board::enterBootloader()`) and the bridge relay (`06`, #365), the
Gateway uploader (#366).

Code: `Firmware/Bootloader/` (the bootloader), `Firmware/Libraries/BootProtocol/BootProtocol.h` (every
wire value below — the one source the bootloader, STM32Board, CANProtocol, the uploader and the
client sync read). Rationale: D26 (core, #363), D27 (safety, #364), D28 (relay + entering, #365), D29 (streaming, #366),
D30 (two builds + freeze, #367).

---

## Flash Layout

| Region | Address | Size | Owner |
|---|---|---|---|
| Bootloader | `0x08000000` | 4 KB (one write-protect block — protected by the bootloader itself) | `Firmware/Bootloader` — replaced only over ST-Link |
| Bootloader info block | `0x08000FF0` | 16 B: magic `"OSBL"` (`0x4C42534F`), protocol version, `transports` (`TRANSPORT_CAN` `0x01` / `TRANSPORT_SERIAL` `0x02`, #367), 10 × `0xFF` | the bootloader (`--section-start`); read by the app (#365) |
| App | `0x08001000` | flash − 4 KB (60 KB C8, 124 KB CB) | every production project (`Firmware/shared/stm32.ini`: `board_build.flash_offset = 0x1000`) |
| App identity block | `0x08001200` | 32 B | inside the app (`08`) |
| PDU calibration page | `0x0800FC00` | 1 KB | PDU builds stop short of it (`board_upload.maximum_size = 64512`); the bootloader erases only the pages an image covers |

The core sets `VTOR` from the offset, so apps need no code change. An ST-Link upload (`pio run -t
upload`) writes the app only and keeps the bootloader. Each build serves the C8 and the CB alike: it
reads the flash size from `0x1FFFF7E0`.

**Two builds, one source (#367, D30).** `Firmware/Bootloader` builds `bootloader` (CAN — every node:
PanelGroup, PDU) and `bridge` (`-DOSH_BOOT_BRIDGE`, USART2 — only the Gateway's PanelBridge, the CAN
master that nothing on the bus can update). Everything but the transport is the same code: the boot
flow, the checks, commit-last, the option bytes, the crash counter, catch-at-boot, the protocol. Both
transports in one image came out ~60 B over 4 KB. The info block's `transports` byte says which one is
installed; a #365 / #366 bootloader has `0xFF` there, which reads as both.

**Blank boards.** A board without the bootloader has nothing at `0x08000000` to start the app, so its
first flash carries both: `pio run -t bootstrap` (dev), or the release's `<board>-<v>-full.hex`. The
app's NODE_ID comes with it — a board gets its ID from its first app (D10 unchanged). A board shipped
with the bootloader only can take its first app over CAN instead (unconfigured, below).

---

## Boot Flow

At every reset:

1. **Entry flag and crash count** — read backup registers `DR1` and `DR2`. `DR1` = `ENTRY_MAGIC`
   (`0xB007`) means the app asked for the bootloader; it is cleared on read. The backup domain survives
   a reset, not a power cycle.
2. **Clocks** — HSE 8 MHz × 9 = 72 MHz, APB1 36 MHz (STM32Board's setup). If the crystal doesn't
   start: HSI / 2 × 9 = 36 MHz with APB1 undivided — still 36 MHz for CAN, so the bit timing holds
   within HSI's ±1 % and the board stays reachable.
3. **App check** — the identity block's magic, layout and length are plausible, the vector table's
   stack pointer is in RAM and its reset vector inside the image, and the hardware CRC over
   `imageLength` (the `imageCrc` word skipped) matches `imageCrc`.
4. **Option bytes** — Data0 / Data1 must equal the valid app's `nodeId` / `nodeKind`, and WRP0 bit 0
   (pages 0–3, the bootloader) must be set to protect. Any difference: rewrite them (below) and reset.
5. **Stay or start**, in this order:

| BootState | When | Announces on |
|---|---|---|
| `REQUESTED` | entry flag | `BOOT_RSP_n`, n = Data0 |
| `UNCONFIGURED` | no valid app, Data0 = `0xFF` | `UNCONFIGURED_RSP_ID` (`0x7C1`) |
| `APP_INVALID` | no valid app, Data0 set — or a BEGIN has erased the app | `BOOT_RSP_n` |
| `APP_UNSTABLE` | `DR2` ≥ `ATTEMPT_LIMIT` (3): the app reset that many times without reporting healthy | `BOOT_RSP_n` |
| `CAUGHT` | a `CONNECT` on `BOOT_CMD_n` within `CATCH_WINDOW_MS` (200 ms) after the reset | `BOOT_RSP_n` |

   With none of these, `DR2` goes up by one and the bootloader **jumps**: SysTick off, CAN / USART2 reset (and its DMA),
   clocks back to the reset state (HSI, PLL and HSE off, zero wait states), NVIC cleared, `VTOR` = app;
   then the stack switch and the branch in **one** inline-asm block (a compiler `pop` between them read
   the app's stack and bus-faulted on the #361 bench).

**Catch at boot** costs every boot 200 ms. It rescues an app that never takes an enter-bootloader
request (CAN broken, hung before READY): the host sends `CONNECT` repeatedly and the cockpit is
power-cycled or the board reset.

**Idle timeout** — `REQUESTED` or `CAUGHT`, the old app still valid, and no command for
`IDLE_TIMEOUT_MS` (30 s): reset into the app. Never after a successful BEGIN (the app is gone) and never for
`APP_UNSTABLE` (it would only crash again).

**Status LEDs — red and green together = "in the bootloader"** (#365). The app never lights both at
once (`docs/firmware/debugging.md`):

| State | LEDs |
|---|---|
| waiting (`REQUESTED`, `CAUGHT`, `APP_INVALID`) | red + green blink together, 1 Hz |
| image arriving | red + green flicker together, ~8 Hz |
| `APP_UNSTABLE` | red + green, alternating with red alone (500 ms) |
| `UNCONFIGURED` | red + green, alternating with green alone (500 ms) |

### Entering from a running app (#365)

A running PanelGroup or PDU listens on its own `BOOT_CMD_n`. A `CONNECT` there calls
`STM32Board::enterBootloader()`: when the info block says a bootloader is installed (and the app runs
behind it, `VTOR` = `0x08001000`) it sets the entry flag (`DR1` = `0xB007`) and resets; without one it
does nothing — a reset would only restart the app. The host simply keeps sending `CONNECT`: the
running app enters, the bootloader answers (`REQUESTED`), and a board being reset is caught the same
way. READY_n byte 3 carries `STM32Board::bootloaderVersion()` (0 = none). `enterBootloader(via)` also
needs the installed bootloader to speak the transport the request came on (`transports`), so a board
with the wrong build ignores the request and keeps running instead of stranding itself. The bridge
never enters on `BOOT_CMD_0`; it has its own path (below).

### Crash-loop counter (`DR2`)

The bootloader adds one before every jump. The app sets it back to 0 once it has booted —
`STM32Board::markBootHealthy()`, called by PanelGroup and PduNode after READY and by PanelBridge at the
end of `setup()`. An app that keeps resetting before that point reaches `ATTEMPT_LIMIT` and the board
stays in its bootloader. A committed image and `GO` both clear it, so the host can try an unstable app
again; a power cycle clears it too (no backup battery on our boards).

### Option bytes — NODE_ID / kind copy and self-protection

The build stays the source of NODE_ID (D10); Data0 / Data1 hold **the ID and kind of the last valid
app**, so a board whose app breaks still answers as its node. WRP0 bit 0 protects the bootloader's own
4 KB. All written by one read-modify-write: read the eight stored option bytes, erase, program them
back changed — RDP first — verify the stored values (value byte only: a stored halfword is
`complement << 8 | value`) and reset, which loads them; `FLASH_WRPR` shows the protection from that
reset on. Written only when something differs (a new board, an ST-Link repurpose, `SET_NODE_ID`); a
failed write leaves them as they were and boot carries on. The bootloader always compares the
*stored* bytes, so it can never rewrite them on every boot.

> **The one dangerous window:** erasing the option bytes also erases RDP, and an erased RDP reads as
> read protection. A power cut between the erase and the RDP write (a few ms, once per new board,
> repurpose or ID change) leaves the chip protected — recover over ST-Link (`stm32f1x unlock 0`, which
> mass-erases), then reflash.

**Replacing the bootloader, or flashing a per-library test sketch at `0x08000000`,** needs the
protection lifted first — in the same halted session, because the bootloader protects itself again at
its next boot:

```
openocd -f interface/stlink.cfg -c "set CPUTAPID 0" -f target/stm32f1x.cfg \
        -c "init; reset halt; flash protect 0 0 0 off; reset halt; shutdown"
```

then `pio run -t upload` (a test) or `pio run -d Firmware/Bootloader -t upload`. RDP, USER and
Data0 / Data1 are kept. `pio run -t bootstrap` afterwards puts bootloader + app back, and the
bootloader re-protects itself on its first boot. (OpenOCD's `stm32f1x unlock` also clears WRP but
keeps Data0 / Data1; `stm32f1x options_write` re-applies the WRP it read.)

### The Gateway's cockpit side — the `bridge` build (#367)

- **Transport:** USART2 at 250 kbit/s (PA2 TX / PA3 RX, the Gateway's RP2040 link), RX by circular
  DMA so bytes keep landing while flash is written. Each CAN frame of the protocol below travels as
  `0xAA, id lo, id hi, len (0–8), data, sum` (`sum` = the bytes after `0xAA`, mod 256), with the id it
  would have had on CAN: commands on `BOOT_CMD_0` (`0x740`), replies and announcements on `BOOT_RSP_0`
  (`0x780`). Commands, replies, the window and the transfer rules are unchanged.
- **Entering:** the host (the Gateway's uploader) writes `BRIDGE_ENTER_EXPORT_ADDRESS` (`0xFF02`) on the
  export stream. PanelBridge calls `enterBootloader(TRANSPORT_SERIAL)` and resets; the bootloader
  announces on USART2 and the uploader sends `CONNECT`.
- **Catch at boot:** the uploader asks the app (`0xFF02`, every 500 ms); once it hears an announcement,
  or after 2 s without one, it sends `CONNECT` every 100 ms, so pressing the Gateway's reset button
  catches a bridge app that never takes the request.
- **The jump** leaves TX idle-high (PA2 input, pull-up) for the app: with USART2 in reset an
  alternate-function pin pulls the line low — a break on the Gateway's UART at every boot.
- **Updated by:** only the Gateway's uploader (`07` § Uploader, target `0`) — the bridge app it carries
  in its own firmware, or one the client staged.

---

## Update Protocol

All frames standard 11-bit, below every control and status frame; only `CTRL_BCAST` (`0x7E0`) is lower (#389, D31). Multi-byte fields little-endian. Byte 0 is the
command (host → bootloader, `BOOT_CMD_n` `0x740+n`) or the reply (bootloader → host, `BOOT_RSP_n`
`0x780+n`).

| Host → bootloader | Bytes 1–7 | Reply |
|---|---|---|
| `CONNECT` `0x01` | — (unconfigured: `uid16:u16`) | `CONNECTED` `0x81` + the announce fields |
| `BEGIN` `0x02` | `len:u32` | `BEGUN` `0x82` `[status]` |
| `BLOCK` `0x10` | `index:u16, crc16:u16` | — (the 10 DATA frames follow) |
| `DATA_k` `0x20+k` (k = 0–9) | 7 bytes of the block, bytes 7k…7k+6 | after `DATA_9`: `BLOCK_ACK` `0x90` `[index:u16, status, next:u16]` |
| `END` `0x30` | `crc:u32` (the identity block's `imageCrc`) | `ENDED` `0xB0` `[status, crc:u32 computed]` — `crc` 0 when END fails before the CRC pass |
| `GO` `0x40` | — | `GOING` `0xC0` `[status]`, then the board resets into the app |
| `SET_NODE_ID` `0x50` | `id` (1–63) | `NODE_SET` `0xD0` `[status]`, then the board resets as node `id` |

`BEGIN`'s `len` is the image length unpadded (the bootloader pads it to whole blocks); a `BEGIN` or
`END` shorter than 5 bytes is ignored, with no reply.

**Announce** (`ANNOUNCE` `0xA0`, every 1 s while no image is arriving — and during a transfer idle for
over 1 s, so the bridge still sees the node alive; `CONNECTED` carries the same),
8 bytes: `[code, PROTOCOL_VERSION, ID (Data0), kind (Data1), BootState, flash KB, uid16:u16]`.
Capacity = flash KB × 1024 − 4 KB. `uid16` = the low half of CRC-32/MPEG-2 over the chip's 96-bit
unique ID (`0x1FFFF7E8`) — tells boards apart (an unconfigured board, or two boards claiming one
NODE_ID).

**Status:** `OK` 0 · `BAD_CRC` 1 · `FLASH_ERROR` 2 · `OUT_OF_RANGE` 3 · `OUT_OF_ORDER` 4 · `NO_APP` 5 ·
`BAD_IMAGE` 6 · `WRONG_NODE` 7 · `WRONG_KIND` 8.

### Transfer rules

- **BEGIN** checks `0 < len` and that the image, padded to whole blocks, fits; then **erases every page
  the image needs — page 0 first, so the old app is invalid from here on** — and only then replies
  (~25 ms per KB: ~0.6 s for 27 KB, ~3 s for 124 KB; it keeps announcing meanwhile). The announced
  state becomes `APP_INVALID`. Pages past the image are never touched.
- **Blocks** are 64 bytes, in order from 0; the image is padded with `0xFF`. CRC-16/CCITT-FALSE over the
  64 bytes (the CDC frames' CRC).
- **Streaming (#366):** a host may send up to `WINDOW_BLOCKS` (4) blocks ahead of the last ack. The
  bootloader polls CAN between its flash writes (and while a reply waits for a transmit mailbox — its
  replies lose arbitration to the host's stream) into a 48-frame RAM ring, so nothing overruns the
  3-frame CAN FIFO. Every block is still acked; `BLOCK_ACK` carries **`next`**, the first block not yet
  written.
- **Losses (go-back-N):** a block already written is acked `OK` (a resend after a lost ack); a block past
  `next` → `OUT_OF_ORDER`, and the host resends from `next`. A `next` lower than one already acked means
  the board restarted and lost the session — the host starts the flash over.
- **Block 0** (the vector table) is held in RAM, not written.
- **END** needs every block. It checks the identity block (in flash, block 8) — magic, `imageLength`
  ≤ the transfer, `imageCrc` = END's CRC, a plausible vector table — then computes the CRC over
  `imageLength` with block 0 from RAM, then the **NODE_ID rule**. All pass: it **writes block 0 last
  (commit)** and clears the crash count — only now does the app pass the boot check. An interrupted
  transfer — at any point — leaves "in bootloader, flashable".
- **GO** with a valid app replies `OK`, clears the crash count and resets; the boot flow starts the app.
- Images without an identity block are refused (`BAD_IMAGE`) — a sketch needs a role library
  (PanelGroup, PowerMonitor, PanelBridge) to have one.

### NODE_ID rule

An upload never changes a board's identity. END refuses an image whose `nodeId` ≠ Data0
(`WRONG_NODE`) or whose `nodeKind` ≠ Data1 (`WRONG_KIND`), before block 0 is committed — the board
stays flashable. The host may offer to change the board's ID first (`SET_NODE_ID`) when the kinds
match; a kind mismatch is always refused. An ST-Link flash is the exception: physical access is the
authority, and the next boot copies the new app's ID (repurpose).

### SET_NODE_ID

Only after a `CONNECT`, only on a configured board, never on the bridge (kind `BRIDGE`, always node 0),
IDs 1–63 — otherwise `OUT_OF_ORDER` / `OUT_OF_RANGE`. It **erases the app's first page first** (a valid
app would put its own ID back at the next boot), then writes Data0 in one option-byte write, replies
and resets: the board comes back as node `id`, `APP_INVALID`, ready for that node's image. A power cut
between the two steps leaves the old ID with no app — still flashable.

### Unconfigured boards

A board with no NODE_ID (Data0 = `0xFF`) and no valid app announces on `UNCONFIGURED_RSP_ID`
(`0x7C1`) and listens on `UNCONFIGURED_CMD_ID` (`0x7C0`). A `CONNECT` there names its `uid16`; only that
board binds the session and answers, and only a bound board takes the commands that follow. Its first
image may carry any `nodeId` — the NODE_ID rule has nothing to compare against — and the next boot
adopts it into Data0 / Data1 (it becomes an ordinary node). **One unconfigured board on the bus at a
time** — a host rule, not a bootloader one: the uploader binds the first announcement it hears.
Announcements are not randomized — a fixed `ANNOUNCE_INTERVAL_MS` (1 s), the first one at once — so
two blank boards powered together can start a frame in the same bit. Same ID, different data: a bit
error, so error frames and a retransmit (the bootloader keeps automatic retransmission), not a lost
board.

---

## Bench Host

`Firmware/Bootloader/tools/flash_can.py` drives the protocol through a CANable (slcan, python-can), or
`--gateway PORT` through the Gateway's USB and the bridge relay (`04`: export records down,
`_BOOT_RSP` lines up), with `--window N` (default 4; 1 = stop-and-wait). The Gateway's own uploader
(#366, `07` § Uploader) runs the same protocol from the RP2040; `upload_gateway.py` is its bench
client (`--bridge` updates the Gateway's own cockpit side, #367). Measured (27–33 KB images): CAN direct 9.5 KB/s stop-and-wait → **15.3 KB/s** with the window;
through the Gateway **12.5 KB/s** writing (≈3.2 s for a whole 27.7 KB update, entering to confirmed). It reads the frame IDs and codes from `BootProtocol.h`, takes the
length and CRC from the image's identity block, sends `CONNECT` every 100 ms while it waits (so
resetting the board catches it), and has `--listen`, `--unconfigured`, `--set-node-id M`,
`--stop-after N` and `--delay-ms` (power-pull tests). Without #365, request the bootloader over
ST-Link instead: write `0xB007` to `DR1` (`0x40006C04`, with the PWR / BKP clocks and `PWR_CR.DBP` on)
and reset.

---

## Freeze (#367, D30)

From firmware 1.0.0 the bootloader is **frozen**: field boards never get a new one over the bus — it is
replaced only over ST-Link — so every later feature lives in the app or the Gateway.

- **Protocol `PROTOCOL_VERSION` 1 is frozen.** The commands, replies, payloads, framing, IDs, info block
  and identity-block checks above don't change. A change to them is a new protocol version, and every
  host must keep speaking version 1 to the boards already out there.
- **The build is pinned:** exact platform, toolchain and CMSIS in `Firmware/Bootloader/platformio.ini`,
  so the same source always builds the same bytes — a newer compiler can move the size by ±100 B.
  Changing any of them is a bootloader change.
- **Budget: 4 KB, never more.** Real footprints at the freeze (vectors, code, `.data` copy, info block):
  `bootloader` 3,684 B (412 free), `bridge` 3,668 B (428 free). The release workflow's summary reports
  both; code that grows into the info block fails the link.
- **A bootloader change** (bug fix only) ships in the release where it changed (`bootloader/` in the
  zip, and every `-full.hex`); `manifest.json` names the release that holds the current one.

**If it ever has to shrink** — least valuable first. Growing past 4 KB is last because it costs app
space on every board.

| Order | Cut | Saves | Lost |
|---|---|---|---|
| 1 | One LED pattern for every state | ~60–80 B | telling the states apart by eye (the client still knows) |
| 2 | The HSI fallback when the crystal is dead | ~40 B | reflashing a board with a dead crystal over the bus |
| 3 | The idle timeout | ~30 B | an abandoned request returns to the app only on `GO` or a power cycle |
| 4 | First image for an unconfigured board | ~100 B | bare boards need ST-Link or come preloaded |
| 5 | Grow to 8 KB | +4 KB | 4 KB of app space on every board |

**Never cut** — these are what keep a bad update from bricking a board: commit-last, the full-image CRC
at boot and at END, the per-block CRC, the NODE_ID / kind rule, the self-protection, the option-byte
verify, the crash-loop counter, and catch-at-boot.
