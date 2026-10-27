# ${BOARD_TITLE} — firmware ${VERSION}

The firmware for the ${BOARD_TITLE} board. It joins the CAN bus as node ${NODE_ID}.

| File | What it holds | Use it for |
|---|---|---|
| `${FULL_FILE}` | the bootloader + the firmware | a new board — never flashed, or fully erased (ST-Link) |
| `${FILE}` | the firmware alone | a board that already has the OpenSkyhawk bootloader (ST-Link) |
| `${BIN_FILE}` | the firmware alone, without a flash address | SkyHawkClient, updating a board that already has the bootloader |

The bootloader lives in the first 4 KB of flash and starts the firmware. It also lets SkyHawkClient
update the board where it is installed.

## Updating over the bus

A board that already has the bootloader doesn't need an ST-Link to update: SkyHawkClient loads
`${BIN_FILE}` onto it through the Gateway, with the board left in the cockpit. You don't open the
`.bin` yourself — it carries no flash address, so for ST-Link use `${FILE}` instead.

## Flashing with an ST-Link

1. Connect an ST-Link to the board's SWD header.
2. In [STM32CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html), connect
   to the ST-Link, open `${FULL_FILE}` on a new board, or `${FILE}` on one that already has the
   bootloader, and click **Download**. The file carries its own flash address, so there is nothing
   to type.

${BOARD_NOTES}

If the ST-Link won't connect, see
https://openskyhawk.github.io/OpenSkyhawk/guides/flashing/#st-link-troubleshooting
