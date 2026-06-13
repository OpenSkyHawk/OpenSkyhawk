# Flashing Firmware

How to get firmware onto a board. STM32 boards use an ST-Link over SWD; RP2040 boards use USB.
PlatformIO drives both.

## STM32 (PanelBridge, PanelGroup) — ST-Link over SWD

Every custom STM32 board exposes a 5-pin SWD header (PA13/PA14/NRST/GND/3.3 V). Connect an
ST-Link and:

```bash
pio run -t upload          # build + flash the default env
pio run -e PanelGroup -t upload   # a specific env
```

`upload_protocol = stlink` is already set in the templates. No separate flasher GUI is needed —
PlatformIO drives the probe directly (STM32CubeProgrammer is optional). See
[PlatformIO Setup](../firmware/platformio-setup.md).

!!! warning "Clone reports 'tap not found'?"
    Many Blue Pill clones use JTAG ID `0x2ba01477` (standard is `0x1ba01477`) and ST-Link
    refuses to connect. Uncomment the override in `platformio.ini`:

    ```ini
    upload_flags =
        -c
        set CPUTAPID 0x2ba01477
    ```

## RP2040 (SimGateway) — USB

No ST-Link needed. Either:

- **From PlatformIO:** `pio run -t upload` — it resets the board into the bootloader and copies
  the firmware over USB, or
- **By hand:** hold BOOTSEL while plugging in, then drag the built `.uf2` onto the
  `RPI-RP2` drive that appears.

## After flashing

Watch the board come up on DiagSerial (115200) and check the status LED. See
[Bring-Up & Testing](bring-up.md) and [Debugging on STM32](../firmware/debugging.md).
