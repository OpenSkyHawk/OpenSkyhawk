# DCS-BIOS Bridge — Tiny2040

Transparent serial relay between the PC (DCS-BIOS @ 250000 baud USB CDC) and
the **STM32 CAN master node** (UART @ 250000 baud). Replaces the Arduino Mega
2560 used during initial prototyping.

In production this role is filled by the RP2040 in the flight stick (see
`Docs/claude/architecture.md`). This board is a standalone prototype bridge
for bench testing before the flight stick is wired up.

## Why this exists

STM32 native USB CDC crashes under sustained DCS-BIOS data flow (port becomes
unusable until replug). The RP2040 USB stack handles 250000 baud without issue.
Full debug notes: `Docs/claude/dcsbios-stm32-debug.md`.

## What it connects

```
PC ──USB CDC── Tiny2040 ──UART── STM32 CAN master node
```

Panel nodes (Center_Armament, etc.) are CAN-only and are downstream of the
STM32 CAN master — they do not connect here.

## Hardware

Any RP2040 module works. Change the `board` in `platformio.ini` to match
what you have on hand (see `Docs/claude/hardware-standards.md` for the
approved module list).

## Wiring

| Tiny2040 pin | Direction | STM32 CAN master pin |
|---|---|---|
| GP0 (UART0 TX) | → | PA3 (RX / UART2) |
| GP1 (UART0 RX) | ← | PA2 (TX / UART2) |
| GND | — | GND |

Both sides are 3.3 V — no level shifter needed.

`Serial1` on the arduino-pico core defaults to UART0 on GP0/GP1. Verify
against your board package version before wiring.

## Firmware

`src/main.cpp` — minimal relay sketch, no external dependencies.

```
PlatformIO env: DCS_BIOS_Bridge
Upload: picotool (hold BOOTSEL while plugging in, then pio run -t upload)
```

## PC side

Point DCS-BIOS at the COM port Windows assigns to the Tiny2040. The
auto-reconnect script stays unchanged — only the COM port number differs
from the Mega.

## Open concern — CAN bus under DCS-BIOS load

See `Docs/claude/dcsbios-stm32-debug.md` for the known open concern about
STM32 UART + CAN bus coexistence under high DCS-BIOS throughput.
