# Bootloader — Technical Specification

**Status:** Done (core, #363) — safety features in #364
**FirmwarePlan ref:** `FirmwarePlan/12-bootloader.md`, `FirmwarePlan/08-hardware-firmware-contracts.md#image-layout--identity-block-362`, `FirmwarePlan/00-decisions.md#d26`
**Depends on:** `BootProtocol`, `FirmwareImage` (headers only); CMSIS device headers

---

## Responsibility

The 4 KB program at `0x08000000` on every STM32 board: check the app and start it, keep the app's
NODE_ID / kind in option bytes Data0 / Data1, and otherwise wait for an image over CAN and write it
safely (commit last). Not an Arduino sketch and not a library — its own PlatformIO project, built
bare-metal (CMSIS, no HAL), C++ without exceptions or RTTI.

Does **not** own: entering on request from a running app or the bridge relay (#365), write protection,
the NODE_ID rule, set-node-ID, first-image adoption, crash-loop counter, catch-at-boot, idle timeout
(#364), the USART2 transport for the bridge (#367).

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
| LED | `led_init()`, `led(red, green)` | PB14 red / PB15 green |
| CAN | `can_init()`, `can_send()`, `can_recv()`, `reply()`, `announce()` | `BTR = 0x033C0003`, ABOM + TXFP; filter 0 = exactly `BOOT_CMD_n` → FIFO0 (disabled when unconfigured); TX mailbox 0, dropped after 20 ms without an ACK |
| Flash | `flash_unlock/wait/erase/program/write()` | 1 KB pages; every halfword read back |
| Option bytes | `option_bytes_write(data0, data1)` | read all 8 → OPTER → program back (RDP first), skip `0xFF` |
| Checks | `crc_reset/feed()`, `identity()`, `identity_ok()`, `app_valid()` | hardware CRC-32/MPEG-2, the `imageCrc` word skipped |
| Jump | `jump_to_app()` | peripherals + clocks to reset state, NVIC cleared, `VTOR`, one asm `msr msp; bx` |
| Session | `Session`, `begin()`, `block()`, `end()`, `handle()` | block 0 in RAM until END; page erase on a page's first block; resend of the last block acked; a good BEGIN sets the announced state to `APP_INVALID` |
| `main()` | entry flag → clocks → app check → Data0/Data1 copy → jump or stay | the stay loop announces every 1 s |

## Implementation Notes

- **Size:** 2,876 B of 4,096 (#363) — ~1.2 KB for #364. RAM ~184 B (`Session` holds two 64-byte buffers).
- **The jump** must switch the stack and branch in one asm block — #361's bus fault came from a
  compiler `pop` after `__set_MSP()`.
- **GO resets** rather than jumping, so the started app always went through the boot check.
- **Option-byte write** is verified by reading the stored halfwords back before the reset; a failed
  write never loops — the app starts with the old Data0 / Data1.
- **Unconfigured** (Data0 = `0xFF`, no app): announces on `0x7C1`, CAN filter off — #364 adds commands.
- **Bench:** verify flash only with the CPU halted — OpenOCD's `verify_image` on a running target loads a
  helper into RAM at `0x20000000` (#361).

## Dependencies

`BootProtocol.h`, `FirmwareImage.h` (its includes `FirmwareVersion.h` / `NodeStatus.h` resolve too);
`framework-cmsis-stm32f1`. Python 3 + python-can for the bench host.
