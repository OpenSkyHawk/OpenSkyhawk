# Build Guides

The how-to section: step-by-step recipes for getting hardware built, flashed, and talking to
DCS. Where the [reference sections](../firmware/index.md) explain *what* things are, these
guides walk you through *doing* them.

## Start here

- **[Your First Panel](first-panel.md)** — the end-to-end walkthrough using the working
  `E2E_DCS_Test` example: one LED, one switch, real DCS-BIOS IDs. Do this first.

## Building a panel

- **[PCB Ordering (JLCPCB)](pcb-ordering.md)** — export fab files and place an order
- **[Assembly & Soldering](assembly.md)** — populate and inspect a board
- **[Flashing Firmware](flashing.md)** — get firmware onto STM32 and RP2040 boards
- **[Bring-Up & Testing](bring-up.md)** — first power-on through a verified DCS connection

## Extending the project

- **[Adding a New Panel Group](new-panel-group.md)** — a new CAN node, NODE_ID first
- **[Adding a New Control Type](new-control-type.md)** — a new input/output firmware class

!!! note "Some guides are partial"
    The assembly and bring-up guides need real build photos, which are still being produced —
    photo placeholders are marked TBD. Kit-specific assembly lives separately under
    [Kit Assembly](../kits/index.md).
