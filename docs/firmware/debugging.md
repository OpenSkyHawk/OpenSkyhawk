# Debugging on STM32

Two things tell you what an STM32 board is doing: the **DiagSerial** debug stream and the
**bi-color status LED**. Neither touches the DCS data path, so you can watch a board in
isolation. This page covers both, plus the bench gotchas that cost the most time.

## DiagSerial — the debug stream

Every STM32 board exposes a debug UART, standard across the project:

| Setting | Value |
|---------|-------|
| Peripheral | USART1 (`Serial1`) |
| Pins | PA9 (TX) / PA10 (RX) |
| Baud | 115200 |
| Header | 3-pin (GND / RX / TX) on every board |

Connect a USB-to-TTL adapter to the 3-pin header and open a serial monitor at 115200
(`pio device monitor`). Enable output by calling `STM32Board::setDebug(true)` in `setup()`;
production builds leave it `false` (default) — the pins stay available but silent.

USART1 is used because PanelBridge already uses UART2 (`Serial`, PA2/PA3) for the RP2040 link —
the two never conflict.

## The status LED

Each custom OpenSkyhawk STM32 board has a bi-color red/green LED on fixed pins (**PB14 = red,
PB15 = green**), initialised automatically by `STM32Board::begin()`. Read the board's state at
a glance:

| LED | Meaning |
|-----|---------|
| Off | Not powered |
| Red blinking | Booting / initialising |
| Green blinking | Normal — CAN healthy, no data flowing |
| **Green solid** | **Connected — CAN healthy and data flowing** (DCS exports on PanelBridge; `CTRL_BCAST` on PanelGroup) |
| Red fast blink | TEC > 0 — transmit errors accumulating |
| Red solid | Bus-off — CAN controller halted |
| Amber flicker (alternating) | Warning / degraded state |

The state machine is shared (`STM32Board`); only the triggers differ per role. **Green solid
(connected)** appears while data is actively flowing and decays back to green-blinking after
~½ s of quiet. **Amber (warning)** means a tracked PanelGroup node died (PanelBridge) or the
master heartbeat was lost (PanelGroup).

CAN faults are driven from the `CanStatus` callback. For non-CAN faults (dead node, lost
master, SYNC timeout, I²C hang), the firmware calls `STM32Board::setWarning(true)` and clears
it with `setWarning(false)` on recovery.

!!! note "PB14/PB15 are reserved"
    The status-LED pins are owned by `STM32Board` — don't use PB14/PB15 for panel I/O or
    MCP23017 interrupts. PC13/PC14/PC15 are also off-limits on custom boards.

## Bench gotchas

!!! warning "Blue Pill clone — ST-Link 'tap not found'"
    Many clones report JTAG ID `0x2ba01477` (standard is `0x1ba01477`) and ST-Link refuses to
    connect. Override it in `platformio.ini`:

    ```ini
    upload_flags =
        -c
        set CPUTAPID 0x2ba01477
    debug_extra_cmds = set CPUTAPID 0x2ba01477
    ```

    The `-c set CPUTAPID` pair must come before the OpenOCD target config loads.

!!! warning "Don't use STM32 USB CDC for DCS-BIOS"
    Enabling STM32 native USB CDC (`PIO_FRAMEWORK_ARDUINO_ENABLE_CDC`) and running DCS-BIOS
    traffic over it **crashes the port** consistently — it becomes unusable until replug. This
    is the whole reason the [SimGateway](../architecture/sim-gateway.md) exists. Use UART; let
    the RP2040 handle USB. (Also: USB and CAN share PA11/PA12 and can't both be active.)

- **Don't remap `Serial` to `Serial1`** via `#define Serial Serial1` — it causes "multiple
  definition of Serial2" or runtime failures depending on the STM32duino version. With no CDC
  flag, `Serial` maps to UART2 (PA2/PA3) natively, which is what PanelBridge uses for the
  RP2040 link.
