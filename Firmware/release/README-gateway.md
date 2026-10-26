# Gateway — firmware ${VERSION}

The Gateway has two chips, and each gets its own file. Flash both, from the same release.

The USB-side file also carries this release's cockpit-side firmware. Once the cockpit side has its
bootloader, a firmware updater can bring it up to date over USB — no ST-Link needed after the first time.

| File | Chip | Side |
|---|---|---|
| `${USB_FILE}` | RP2040 | USB side — talks to the PC |
| `${COCKPIT_FULL_FILE}` | STM32F103C8 | Cockpit side — the bootloader + the firmware, for a new board |
| `${COCKPIT_FILE}` | STM32F103C8 | Cockpit side — the firmware alone, for a board that already has the bootloader |

## 1. USB side (RP2040)

1. Close SkyHawkClient and any serial monitor — they hold the USB port.
2. Hold the **BOOTSEL** button while plugging the Gateway into USB. A drive named `RPI-RP2`
   appears.
3. Drag `${USB_FILE}` onto the `RPI-RP2` drive. The drive disappears and the Gateway restarts.

## 2. Cockpit side (STM32)

1. Connect an ST-Link to the Gateway's SWD header.
2. In [STM32CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html), connect
   to the ST-Link, open `${COCKPIT_FULL_FILE}` on a new board, or `${COCKPIT_FILE}` on one that
   already has the OpenSkyhawk bootloader, and click **Download**. The file carries its own flash
   address, so there is nothing to type.

If the ST-Link won't connect, see
https://openskyhawk.github.io/OpenSkyhawk/guides/flashing/#st-link-troubleshooting
