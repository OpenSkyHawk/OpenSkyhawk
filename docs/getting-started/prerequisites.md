---
status: new
---

# What You'll Need

What you need depends on what you're here to do. Pick your path — you don't need the
build or firmware toolchain just to understand the project.

This page lists the tools and skills. It does **not** cover installation — setup steps live
in the [build guides](../guides/index.md).

## To understand the project

You can read the whole site and follow the architecture with nothing installed. To actually
*see* OpenSkyhawk drive a cockpit, you need:

- **[DCS World](https://www.digitalcombatsimulator.com/)** with the **A-4E-C Skyhawk
  Community Mod** installed. The A-4E-C is a free community module — OpenSkyhawk targets it
  specifically.
- **Basic electronics familiarity** — you should be comfortable with the idea of a
  microcontroller, a serial link, and reading a schematic. You do not need to know CAN bus,
  DCS-BIOS, or STM32 development going in; the docs explain those.

## To build a panel

Fabricating and wiring a panel adds the hardware toolchain:

| Tool | Use | Notes |
|------|-----|-------|
| **[KiCad](https://www.kicad.org/)** 8+ | PCB schematic + layout review | Project tooling and CI use **v10.0.1**; open the `.kicad_pro` projects under `PCB/` |
| **[PlatformIO](https://platformio.org/)** + **[VS Code](https://code.visualstudio.com/)** | Firmware build and upload | The firmware build system — see [PlatformIO Setup](../firmware/platformio-setup.md) |
| **[STM32CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html)** + ST-Link | Flashing STM32 boards over SWD | STM32 boards expose a 5-pin SWD header (PA13/PA14/NRST/GND/3.3V) |
| **[DCS-BIOS](https://github.com/DCS-Skunkworks/dcs-bios)** | DCS ↔ cockpit export stream | Runs on the PC; the firmware speaks its protocol |
| **[JLCPCB](https://jlcpcb.com/) account** | PCB fabrication | Design rules are pre-loaded for JLCPCB's standard 2-layer service |
| **Soldering iron + multimeter** | Assembly and bring-up | Surface-mount rework; continuity and voltage checks |

CAD is optional for panel building — the **`.f3d`** Fusion 360 sources under `CAD/` open in
**[Autodesk Fusion 360](https://www.autodesk.com/products/fusion-360/)** if you want to print
or modify the physical panels.

!!! note "RP2040 boards need no special flasher"
    The SimGateway and any HID boards run on off-the-shelf RP2040 modules (e.g. a Raspberry
    Pi Pico). They flash over USB by drag-and-drop UF2 or directly from PlatformIO — no
    ST-Link required. Only the STM32 boards need the SWD programmer.

## To contribute firmware

Everything above, plus the bench hardware to develop and verify against real boards:

- **STM32F103CBT6 dev board** — a Blue Pill is fine for development. Note the firmware
  requires an **external 8 MHz crystal** for CAN; verify your board has one populated.
- **RP2040 module** (Raspberry Pi Pico or similar) — for SimGateway work.
- **USB-to-TTL serial adapter** — for the DiagSerial debug stream. Every STM32 board exposes
  a 3-pin header (GND / RX / TX) on **USART1 at 115200 baud** for human-readable diagnostics.
- **ST-Link** (or compatible SWD probe) — for flashing and debugging STM32 targets.
- **CAN bus analyzer** — *optional but useful*. Lets you watch frames on the bus directly
  when debugging multi-node behaviour.

See [How to Contribute](../contributing/index.md) for conventions, and
[Adding a New Panel Group](../guides/new-panel-group.md) for the end-to-end workflow.
