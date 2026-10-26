# Bootloader

Every OpenSkyhawk STM32 board (PanelGroup nodes, PDUs, and the Gateway's PanelBridge chip) carries a
small bootloader in its first 4 KB of flash. It starts the firmware, and it lets the board take a new
firmware image over the cockpit's CAN bus, so a board can be updated without an ST-Link.

## What it does at power-up

The bootloader checks the firmware before starting it. Every firmware image carries a small identity
block: which node it is for, what kind of board, its version, and a checksum over the whole image. If
the checksum matches, the firmware starts within a few milliseconds and the board behaves as it always
has. If it doesn't — an update was interrupted, or the board was never flashed — the bootloader stays
in charge, blinks the red and green LEDs together, and announces itself on the bus once a second,
ready for a new image. Red and green lit together always means "in the bootloader" — the firmware
never shows that pattern; the full list is on the [status LED](debugging.md#in-the-bootloader-red-and-green-together)
page.

If the firmware passes the check but keeps resetting before it finishes booting, the bootloader
counts the attempts. After three, it stops starting it and announces the board as unstable (red and
green together, alternating with red alone) until a new image arrives. And for 200 ms after every reset it listens for an update tool
asking it to stay — so even firmware that never answers on the bus can be replaced: power-cycle the
cockpit while the update is waiting.

An update is written so that a failure at any point is safe. The first block of the new firmware is
held back and written only after the whole image has arrived and its checksum matches. Pull the power
halfway through, and the board comes back in its bootloader, ready to try again.

## Updating through the cockpit

A board doesn't need its own connection for an update. The firmware updater reaches it the way the
simulator does: from the PC through the Gateway's USB, and across the CAN bus. The PC hands the whole
image to the Gateway, and the Gateway does the rest on its own: it asks the board to restart into its
bootloader, checks that the image is for that node and that kind of board before erasing anything,
writes it, and confirms the board comes back running the new version. If the PC goes to sleep or the
cable comes out halfway, the Gateway still finishes and keeps the result for when the PC is back; if
the transfer itself fails, it tries again.

An update of a typical board takes about three seconds — around six for one with a full 60 KB of
firmware. The Gateway's PanelBridge reports each board's state as it goes — running, waiting in its
bootloader, or being updated — along with the bootloader version it has.

## How a board knows its node number

A board's NODE_ID still comes from the firmware you build (`-DNODE_ID` in `platformio.ini`, see
[NODE_ID & CAN Addressing](node-id.md)). The bootloader keeps a copy of it, so a board whose firmware
is broken still answers as its own node. Flash a different node's firmware over ST-Link, and the copy
follows on the next boot.

An update over the bus never changes a board's node number. Firmware built for node 5 is refused by
node 3; changing the number is a separate step the update tool offers, and firmware for a different
kind of board (a PDU image on a panel board) is always refused. A board that came with the bootloader
alone has no node number yet: it takes its first firmware over the bus — one such board on the bus at
a time — and becomes that firmware's node.

## Getting the bootloader onto a board

A new board needs the bootloader and its firmware together, once:

```bash
pio run -t bootstrap
```

Run it from the board's project folder. It builds and flashes the bootloader, then this project's
firmware. After that, `pio run -t upload` updates the firmware alone and leaves the bootloader in
place.

From a release, flash the board's `-full.hex` file (bootloader plus firmware) on a new board, and its
`.elf` file (firmware alone) on a board that already has the bootloader. See
[Flashing Firmware](../guides/flashing.md).

!!! note "Your own sketch works too"
    The bootloader starts any firmware with a valid identity block, and every sketch built on the
    PanelGroup library gets one automatically. A bare sketch that uses none of the OpenSkyhawk node
    libraries has no identity block, and the bootloader won't start it.

## The bootloader protects itself

On its first boot the bootloader write-protects its own 4 KB, so nothing — not a stray write, not a
mistaken upload — can damage it. Flashing anything at address 0 therefore needs the protection lifted
first: replacing the bootloader, or running one of the per-library test sketches under
`Firmware/Tests/`, which flash from address 0. With the ST-Link connected:

```bash
openocd -f interface/stlink.cfg -c "set CPUTAPID 0" -f target/stm32f1x.cfg -c "init; reset halt; flash protect 0 0 0 off; reset halt; shutdown"
```

Then upload as usual (`pio run -t upload` in the test project). The board's node number is kept.
To go back, run `pio run -t bootstrap` from the board's project: the bootloader returns and protects
itself again on its first boot.
