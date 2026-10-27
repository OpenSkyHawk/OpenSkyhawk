# Gateway — firmware ${VERSION}

The Gateway has two chips, each with its own files. Keep both on the same release.

The USB-side file also carries this release's cockpit-side firmware. Once the cockpit side has its
bootloader, SkyHawkClient can bring both sides up to date — no ST-Link needed after the first
time.

| File | Chip | Use it for |
|---|---|---|
| `${USB_FILE}` | RP2040 | USB side — talks to the PC |
| `${COCKPIT_FULL_FILE}` | STM32F103C8 | Cockpit side — the bootloader + the firmware, for a new board (ST-Link) |
| `${COCKPIT_FILE}` | STM32F103C8 | Cockpit side — the firmware alone, for a board that already has the bootloader (ST-Link) |
| `${COCKPIT_BIN_FILE}` | STM32F103C8 | Cockpit side — the firmware alone, without a flash address, for SkyHawkClient |

## Updating over the bus

A Gateway that already runs OpenSkyhawk firmware, with the bootloader on its cockpit side, needs no
buttons and no ST-Link: SkyHawkClient updates it. The client restarts the USB side into its update
drive itself, then brings the cockpit side up to date. You don't open `${COCKPIT_BIN_FILE}` yourself
— it carries no flash address, so for ST-Link use `${COCKPIT_FILE}` instead.

## Flashing by hand

### 1. USB side (RP2040)

1. Close SkyHawkClient and any serial monitor — they hold the USB port.
2. Plug the Gateway into USB. A blank Gateway shows up as a drive named `RPI-RP2` by itself.
3. Drag `${USB_FILE}` onto the `RPI-RP2` drive. The drive disappears and the Gateway restarts.

A Gateway that is already running is updated by SkyHawkClient (above), which restarts it into the
`RPI-RP2` drive. Hold the **BOOTSEL** button while plugging it in only if its firmware hangs before
USB comes up, so neither the drive nor the client can find it.

### 2. Cockpit side (STM32)

1. Connect an ST-Link to the Gateway's SWD header.
2. In [STM32CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html), connect
   to the ST-Link, open `${COCKPIT_FULL_FILE}` on a new board, or `${COCKPIT_FILE}` on one that
   already has the OpenSkyhawk bootloader, and click **Download**. The file carries its own flash
   address, so there is nothing to type.

If the ST-Link won't connect, see
https://openskyhawk.github.io/OpenSkyhawk/guides/flashing/#st-link-troubleshooting
