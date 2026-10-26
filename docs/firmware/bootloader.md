# Bootloader

Every OpenSkyhawk STM32 board (PanelGroup nodes, PDUs, and the Gateway's PanelBridge chip) carries a
small bootloader in its first 4 KB of flash. It starts the firmware, and it lets the board take a new
firmware image over the cockpit's CAN bus, so a board can be updated without an ST-Link.

## What it does at power-up

The bootloader checks the firmware before starting it. Every firmware image carries a small identity
block: which node it is for, what kind of board, its version, and a checksum over the whole image. If
the checksum matches, the firmware starts within a few milliseconds and the board behaves as it always
has. If it doesn't — an update was interrupted, or the board was never flashed — the bootloader stays
in charge, blinks the status LED red, and announces itself on the bus once a second, ready for a new
image.

An update is written so that a failure at any point is safe. The first block of the new firmware is
held back and written only after the whole image has arrived and its checksum matches. Pull the power
halfway through, and the board comes back in its bootloader, ready to try again.

## How a board knows its node number

A board's NODE_ID still comes from the firmware you build (`-DNODE_ID` in `platformio.ini`, see
[NODE_ID & CAN Addressing](node-id.md)). The bootloader keeps a copy of it, so a board whose firmware
is broken still answers as its own node. Flash a different node's firmware over ST-Link, and the copy
follows on the next boot.

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

!!! warning "A full chip erase removes the bootloader"
    So do the per-library test sketches under `Firmware/Tests/`, which flash from address 0. Put it
    back with `pio run -d Firmware/Bootloader -t upload`, or `pio run -t bootstrap` from a project.
