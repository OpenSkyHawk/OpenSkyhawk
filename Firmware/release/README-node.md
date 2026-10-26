# ${BOARD_TITLE} — firmware ${VERSION}

The firmware for the ${BOARD_TITLE} board. It joins the CAN bus as node ${NODE_ID}.

| File | What it holds | Use it for |
|---|---|---|
| `${FULL_FILE}` | the bootloader + the firmware | a new board — never flashed, or fully erased |
| `${FILE}` | the firmware alone | a board that already has the OpenSkyhawk bootloader |

The bootloader lives in the first 4 KB of flash and starts the firmware. It also lets the board take
firmware updates over the cockpit's CAN bus.

## Flashing

1. Connect an ST-Link to the board's SWD header.
2. In [STM32CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html), connect
   to the ST-Link, open the file from the table above and click **Download**. The file carries its
   own flash address, so there is nothing to type.

${BOARD_NOTES}

If the ST-Link won't connect, see
https://openskyhawk.github.io/OpenSkyhawk/guides/flashing/#st-link-troubleshooting
