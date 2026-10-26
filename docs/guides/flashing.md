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

**A new board needs the [bootloader](../firmware/bootloader.md) first.** The firmware starts at
`0x08001000`, behind the bootloader, so a board that has never been flashed gets both at once:

```bash
pio run -t bootstrap       # bootloader + firmware, once per new board
```

After that, `pio run -t upload` flashes the firmware alone and keeps the bootloader. From a release,
use the board's `-full.hex` file on a new board and its `.elf` file on one that already has the
bootloader.

The bootloader write-protects itself, so anything that writes from address 0 — the per-library test
sketches, or a new bootloader — needs the protection lifted first. The one-line command is on the
[Bootloader](../firmware/bootloader.md#the-bootloader-protects-itself) page.

### ST-Link troubleshooting

!!! warning "Upload fails with 'only supports deprecated HLA'?"
    The ST-Link's own firmware is too old — PlatformIO's OpenOCD needs **V2J24 or newer**, and
    many clones ship older. Update the probe with ST's free
    [STSW-LINK007](https://www.st.com/en/development-tools/stsw-link007.html) updater: plug the
    ST-Link in on its own, click **Open in update mode**, then **Upgrade**, and replug.
    On macOS the updater needs Java (`brew install --cask temurin`); without it the app closes
    as soon as it opens.

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

No ST-Link needed — the board flashes over the same USB cable it runs on. Either:

- **From PlatformIO:** `pio run -t upload`. PlatformIO resets the board into its bootloader by
  itself (the 1200-baud touch, which the Adafruit TinyUSB library in every SimGateway build
  handles), then flashes it.
- **By hand:** hold BOOTSEL while plugging the board in (or while tapping RESET), wait for the
  `RPI-RP2` drive, and drag the `.uf2` onto it.

!!! warning "Upload can't find the board?"
    Something is holding the CDC port open. Close SkyHawkClient, serial monitors, and any
    browser HID/serial page before flashing — an orphaned Electron process is enough to keep the
    port claimed; check with `lsof /dev/cu.usbmodem*`.

    The automatic reset needs the running firmware to answer on USB. If the board's firmware
    hangs before USB comes up, put it into BOOTSEL by hand and run the upload again.

## After flashing

Watch the board come up on DiagSerial (115200) and check the status LED. See
[Bring-Up & Testing](bring-up.md) and [Debugging on STM32](../firmware/debugging.md).
