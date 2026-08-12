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
        set CPUTAPID 0
    ```

    Use `0`, not the clone's actual ID. `0` disables the ID check outright. Setting it to
    `0x2ba01477` gets past the initial check but has been observed to fail part-way through
    the write — the projects under `Firmware/Tests/` all use `0` for this reason.

## RP2040 (SimGateway) — USB

No ST-Link needed, but **put the board into BOOTSEL by hand first**: hold BOOTSEL while plugging
it in (or while tapping RESET), and wait for the `RPI-RP2` drive to appear. Then either:

- **From PlatformIO:** `pio run -t upload`, or
- **By hand:** drag the built `.uf2` onto the `RPI-RP2` drive.

!!! warning "`pio run -t upload` cannot reset the board for you"
    Every SimGateway project builds with `-DUSE_TINYUSB`, which replaces the Pico core's USB
    stack — and the 1200-baud-touch auto-reset goes with it. Without a manual BOOTSEL,
    `picotool` finds no device in bootloader mode and the upload fails.

    Anything holding the CDC port open also blocks the upload — close SkyHawkClient, serial
    monitors, and any browser HID/serial page before flashing. An orphaned Electron process
    is enough to keep the port claimed; check with `lsof /dev/cu.usbmodem*` if BOOTSEL alone
    doesn't fix it.

## After flashing

Watch the board come up on DiagSerial (115200) and check the status LED. See
[Bring-Up & Testing](bring-up.md) and [Debugging on STM32](../firmware/debugging.md).
