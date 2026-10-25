# ${BOARD_TITLE} — firmware ${VERSION}

`${FILE}` is the firmware for the ${BOARD_TITLE} board. It joins the CAN bus as node ${NODE_ID}.

## Flashing

1. Connect an ST-Link to the board's SWD header.
2. In [STM32CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html), connect
   to the ST-Link, open `${FILE}` and click **Download**. The file carries its own flash address,
   so there is nothing to type.

${BOARD_NOTES}

If the ST-Link won't connect, see
https://openskyhawk.github.io/OpenSkyhawk/guides/flashing/#st-link-troubleshooting
