# Bootloader — Technical Specification

**Status:** Done (core #363, safety #364, info block + LEDs #365)
**FirmwarePlan ref:** `FirmwarePlan/12-bootloader.md`, `FirmwarePlan/08-hardware-firmware-contracts.md#image-layout--identity-block-362`, `FirmwarePlan/00-decisions.md#d26`, `#d27`
**Depends on:** `BootProtocol`, `FirmwareImage` (headers only); CMSIS device headers

---

## Responsibility

The 4 KB program at `0x08000000` on every STM32 board: check the app and start it, keep the app's
NODE_ID / kind in option bytes Data0 / Data1, write-protect itself, count app boots that never report
healthy, catch a CONNECT in the 200 ms after a reset, and otherwise wait for an image over CAN and
write it safely (commit last, NODE_ID rule). Not an Arduino sketch and not a library — its own PlatformIO project, built
bare-metal (CMSIS, no HAL), C++ without exceptions or RTTI.

Does **not** own: entering on request from a running app or the bridge relay (#365), the USART2
transport for the bridge (#367), clearing the crash-loop counter (the app, via
`STM32Board::markBootHealthy()`).

## File Layout

```
Firmware/Bootloader/
├── platformio.ini        env:bootloader — framework cmsis, -Os, --gc-sections,
│                         board_upload.maximum_size = 4096 (the build fails past it),
│                         -I to BootProtocol / FirmwareImage / FirmwareVersion / NodeStatus
├── src/main.cpp          the whole bootloader
└── tools/flash_can.py    bench host (python-can + CANable), reads BootProtocol.h
```

Production projects reach it through `Firmware/shared/`: `stm32.ini` sets the app offset, and
`osh_image.py` adds the `bootstrap` target (build + upload this project, then the app).

## Structure (`main.cpp`)

| Part | Functions | Notes |
|---|---|---|
| Clocks | `clock_init()` → SYSCLK Hz, `clock_deinit()` | HSE 72 MHz with a start timeout, else HSI/2 × 9 = 36 MHz (APB1 36 MHz either way — same CAN timing) |
| LED | `led_init()`, `led(red, green)` | PB14 red / PB15 green; red + green together = in the bootloader (patterns in FirmwarePlan 12) |
| Info block | `osh_bootloader_info` | `.osh_bootinfo` at `0x08000FF0` (`-Wl,--section-start`), kept by `-Wl,-u` — code growing into it fails the link |
| CAN | `can_init()`, `can_send()`, `can_recv()`, `reply()`, `announce()` | `BTR = 0x033C0003`, ABOM + TXFP; filter 0 = exactly `BOOT_CMD_n` → FIFO0 (disabled when unconfigured); TX mailbox 0, dropped after 20 ms without an ACK |
| Flash | `flash_unlock/wait/erase/program/write()` | 1 KB pages; every halfword read back |
| Option bytes | `option_bytes_set(data0, data1, wrp0)`, `wrp0()` | read all 8 → OPTER → program back (RDP first), skip `0xFF`, verify the stored value bytes |
| Backup registers | `bkp(n)` | DR1 entry flag, DR2 crash-loop count |
| Checks | `crc_reset/feed()`, `identity()`, `identity_ok()`, `app_valid()`, `uid16()` | hardware CRC-32/MPEG-2, the `imageCrc` word skipped; UID hash over `0x1FFFF7E8` |
| Jump | `jump_to_app()` | peripherals + clocks to reset state, NVIC cleared, `VTOR`, one asm `msr msp; bx` |
| Session | `Session`, `begin()`, `block()`, `end()`, `set_node_id()`, `handle()` | block 0 in RAM until END; page erase on a page's first block; resend of the last block acked; a good BEGIN sets `APP_INVALID`; END applies the NODE_ID rule and clears DR2; unconfigured: only a CONNECT naming our UID16 binds |
| `main()` | entry flag + DR2 → clocks → app check → option bytes → state → catch window → jump or stay | the stay loop announces every 1 s and resets into a still-valid app after 30 s idle |

## Implementation Notes

- **Size:** 3,560 B after #365 (incl. the 16-byte info block; 3,428 B after #364, 2,876 B after #363).
  Code ends at `0x08000EF0`; the hard limit for code is `0x08000FF0`. RAM ~196 B (`Session` holds two
  64-byte buffers).
- **Idle announce (#365):** the stay loop also announces during a transfer quiet for over 1 s, so the
  bridge keeps the node alive.
- **The jump** must switch the stack and branch in one asm block — #361's bus fault came from a
  compiler `pop` after `__set_MSP()`.
- **GO resets** rather than jumping, so the started app always went through the boot check.
- **Option-byte write** is verified by reading the stored value bytes back before the reset (a stored
  halfword is `complement << 8 | value` — #363 compared the whole halfword, so its verify always failed
  and it skipped the reset; harmless there, fixed in #364). A failed write never loops — boot carries on
  with the old values. The reset loads them: `FLASH_WRPR` shows the protection right after it.
- **Unconfigured** (Data0 = `0xFF`, no app): filter and replies on `0x7C0` / `0x7C1`; a session needs a
  CONNECT naming our UID16.
- **Bench:** verify flash only with the CPU halted — OpenOCD's `verify_image` on a running target loads a
  helper into RAM at `0x20000000` (#361).

## Dependencies

`BootProtocol.h`, `FirmwareImage.h` (its includes `FirmwareVersion.h` / `NodeStatus.h` resolve too);
`framework-cmsis-stm32f1`. Python 3 + python-can for the bench host.
