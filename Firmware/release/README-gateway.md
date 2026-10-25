# Gateway — firmware ${VERSION}

The Gateway has two chips, and each gets its own file. Flash both, from the same release.

| File | Chip | Side |
|---|---|---|
| `${USB_FILE}` | RP2040 | USB side — talks to the PC |
| `${COCKPIT_FILE}` | STM32F103C8 | Cockpit side — talks to the panels over CAN |

## 1. USB side (RP2040)

1. Close SkyHawkClient and any serial monitor — they hold the USB port.
2. Hold the **BOOTSEL** button while plugging the Gateway into USB. A drive named `RPI-RP2`
   appears.
3. Drag `${USB_FILE}` onto the `RPI-RP2` drive. The drive disappears and the Gateway restarts.

## 2. Cockpit side (STM32)

1. Connect an ST-Link to the Gateway's SWD header.
2. In [STM32CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html), connect
   to the ST-Link, open `${COCKPIT_FILE}` and click **Download**. The file carries its own flash
   address, so there is nothing to type.

If the ST-Link won't connect, see
https://openskyhawk.github.io/OpenSkyhawk/guides/flashing/#st-link-troubleshooting
