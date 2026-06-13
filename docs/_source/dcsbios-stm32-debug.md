# DCS-BIOS STM32 Debug Session Notes

Findings from initial DCS-BIOS integration testing with a Blue Pill prototype.
These notes apply to any STM32 node that will run DCS-BIOS — in this project
that is the **STM32 CAN master node**, not individual panel nodes like
Center_Armament.

## Architecture Context

Panel nodes (e.g. Center_Armament) are CAN-only. They never touch DCS-BIOS
directly. The data flow is:

```
PC ──USB CDC── RP2040 (gateway) ──UART── STM32 CAN master ──CAN bus── panel nodes
```

During prototyping, a Tiny2040 stood in for the RP2040 gateway and a Blue Pill
stood in for the STM32 CAN master. The bridge firmware (`DCS_BIOS_Bridge`) was
removed in the prototype cleanup — see git history if needed.

## Blue Pill (STM32F103C8T6) Gotchas

### JTAG ID — Chinese clone

Standard STM32 JTAG ID: `0x1ba01477`  
Chinese Blue Pill clone: `0x2ba01477`

ST-Link refuses to connect without overriding the ID. Add to `platformio.ini`:

```ini
upload_flags =
    -c
    set CPUTAPID 0x2ba01477
debug_extra_cmds = set CPUTAPID 0x2ba01477
```

Flag ordering matters — the `-c set CPUTAPID` pair must appear before the
OpenOCD target config loads.

### USB CDC crashes under DCS-BIOS

`PIO_FRAMEWORK_ARDUINO_ENABLE_CDC` enables STM32 native USB. Under sustained
DCS-BIOS data flow the port crashes: socat exits with "Permission denied" and
the port is unusable until replug. This happened consistently — it is not a
fluke.

**Do not use STM32 USB CDC for DCS-BIOS.** Use UART and bridge via RP2040.

### UART1 redefinition conflicts

Attempting to remap `Serial` to `Serial1` (PA9/PA10) via `#define Serial
Serial1` causes "multiple definition of Serial2" compile errors or runtime
failures depending on the STM32duino version in use.

## What Works — UART2 + RP2040 Bridge

On `board = genericSTM32F103C8` with **no CDC flag**, `Serial` maps to
**UART2**:

| Object | UART | TX pin | RX pin |
|--------|------|--------|--------|
| `Serial` | UART2 | PA2 | PA3 |

Required defines before `#include <DCSBIOS.h>`:

```cpp
#define DCSBIOS_FOR_STM32
#define DCSBIOS_DEFAULT_SERIAL
```

Baud rate: **250000** (fixed by DCS-BIOS protocol).

### Arduino Mega 2560 bridge wiring (used in this session)

STM32 UART2 connects to Mega UART1. Mega relays traffic between UART1 and
its USB Serial (`Serial` / UART0) at 250000 baud on both sides.

| Arduino Mega | Direction | STM32 |
|---|---|---|
| Pin 18 (TX1 / UART1) | → | PA3 (UART2 RX) |
| Pin 19 (RX1 / UART1) | ← | PA2 (UART2 TX) |
| GND | — | GND |

⚠️ STM32 is 3.3 V; Mega is 5 V. The STM32 RX pin (PA3) receives 5 V from
the Mega TX — this is out-of-spec for STM32 GPIO. It worked on the bench
but a 5 V→3.3 V voltage divider or level shifter on that line is recommended
for anything beyond a one-off proto session.

The Mega's TX line (Mega Pin 18 → STM32 PA3) drives 5 V into a 3.3 V input.

**Planned replacement:** Tiny2040 (or any RP2040 module) — both sides 3.3 V,
no level shifter needed.

### PC13 onboard LED

Active-low on Blue Pill:

```cpp
digitalWrite(PC13, LOW);   // on
digitalWrite(PC13, HIGH);  // off
```

## DCS-BIOS Verified Addresses

| Signal | Address | Mask | Shift |
|--------|---------|------|-------|
| Master caution | `0x7408` | `0x0200` | 9 |

## Status at End of Session

- Connection stable under sustained DCS-BIOS flow (no drops vs. constant USB CDC crashes)
- Master caution LED toggled correctly on bench
- End-to-end verification in DCS (sim running) not yet done

## Open Concern — CAN Bus Under DCS-BIOS Load

Not yet tested. The STM32 CAN master must process incoming UART bytes from the
RP2040 and simultaneously handle CAN interrupts. Under high DCS-BIOS throughput,
UART buffer overrun or CAN message loss is possible.

Mitigations to evaluate when CAN integration begins:

- Use DMA for UART Rx on STM32 (frees CPU from byte-by-byte polling)
- Throttle DCS-BIOS subscriptions to only outputs actually needed
- Increase UART baud rate (115200 → 500000+) to reduce buffer accumulation
- Verify CAN interrupt latency under worst-case DCS-BIOS burst traffic
