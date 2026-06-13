# PlatformIO Setup

OpenSkyhawk firmware builds with [PlatformIO](https://platformio.org/). Each board is its own
project. Start from the templates in `Firmware/Templates/` — **not** from an existing panel's
`platformio.ini`, which may carry stale dependencies.

## Start from a template

`Firmware/Templates/` has a ready project per tier: `PanelGroup/`, `PanelBridge/`, and
`SimGateway/`. Copy the one you need into place (a PanelGroup goes under
`Firmware/Panels/<YourPanel>/`) and edit its `platformio.ini`.

## A PanelGroup platformio.ini, annotated

```ini
[env:PanelGroup]
platform = ststm32
board = genericSTM32F103C8        ; STM32F103C8 for nodes (see board note below)
framework = arduino
board_build.f_cpu = 72000000L

build_flags =
    -DNODE_ID=1                   ; unique per board, 1–63 — set this!
    -DHAL_CAN_MODULE_ENABLED      ; enable the bxCAN peripheral
    -DUSB_NONE                    ; USB off — CAN owns PA11/PA12
    -DHSE_VALUE=8000000           ; external 8 MHz crystal, required for CAN timing

lib_extra_dirs = ${PROJECT_DIR}/../../Libraries
lib_ldf_mode = deep+              ; needed so Adafruit BusIO finds framework headers
lib_deps =
    HIDControls
    STM32Board
    PanelGroup
    CANProtocol
    A4EC
    blemasle/MCP23017@^2.0.0
    adafruit/Adafruit ADS1X15@^2.0.0

upload_protocol = stlink
monitor_speed = 115200
```

The two build flags people forget: **`-DNODE_ID`** (every board needs a unique value — see
[NODE_ID & CAN Addressing](node-id.md)) and **`-DHSE_VALUE=8000000`** (selects the external
crystal; the internal RC oscillator is not accurate enough for 500 kbps CAN).

!!! note "Which STM32 variant"
    Per the [variant policy](../getting-started/prerequisites.md), PanelGroup nodes default to
    **`genericSTM32F103C8`** (64 KB) and **PanelBridge** uses **`genericSTM32F103CB`** (128 KB,
    for the DCS-BIOS input map). Note: the in-tree `Templates/PanelGroup` currently still
    targets `genericSTM32F103CB` — set it to `C8` unless your panel actually needs the extra
    flash.

## Flashing

STM32 boards flash over **SWD with an ST-Link** (`upload_protocol = stlink`). PlatformIO drives
the probe directly — `pio run -t upload`. SimGateway (RP2040) flashes over USB by UF2 or
straight from PlatformIO; no ST-Link.

!!! warning "Blue Pill clone won't connect? Override the tap ID"
    Many clones report a non-standard JTAG ID and ST-Link refuses to connect with "tap not
    found." Uncomment the override in the template:

    ```ini
    upload_flags =
        -c
        set CPUTAPID 0x2ba01477
    ```

    The standard ID is `0x1ba01477`; the common clone is `0x2ba01477`.

## Serial monitor

`monitor_speed = 115200` matches the **DiagSerial** debug stream (USART1, PA9/PA10). Attach a
USB-to-TTL adapter to the board's 3-pin debug header and `pio device monitor`. See
[Debugging on STM32](debugging.md).
