# Right_Navigation — System Interconnect / Harness Schematic

**Documentation only — NOT a fabricated board.** No PCB, no gerbers, no DRC. This KiCad
project is the controller-level **interconnect / harness diagram** for the Right_Navigation
panel group (NODE_ID 2): a block diagram with one block per board, wired to show the harness
nets between them.

## Why it exists

KiCad can't live-link separate projects, and the three boards
(`ASN-41` host, `APN-153_Doppler`, `ARC-51`) are each their own project. This sheet is the
**interface contract**: it defines the harness pinouts once, and each board's connector
conforms to it. It is also the **harness build/wiring document** for B7/B8.

It is the visual + electrically-checkable form of the I/O architecture described on the
controller issue [#168](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/168).

## What it contains (to be drawn)

- **Block symbol per board** — ASN-41 (host/hub), APN-153, ARC-51 — each exposing its
  harness connector (J1) pins.
- **Harness nets:**
  - Shared SPI bus (if the shift-register path is adopted): `SCK · SER · SH_LD`
    + per-board selects `MISO_ASN / MISO_APN / MISO_ARC` + `RCLK_APN` (no inter-board
    daisy-chain — shared bus + per-board select).
  - I²C OLED buses (host TCA9548A per bus → mux channels, OLEDs fixed 0x3C, 1/channel).
  - `CAN_H/L`, `+5V`, `+3V3`, `+12V` (backlight), `GND`, `VOL` analog.
  - Two harness legs: **Bus A (I²C1) → APN-153**, **Bus B (I²C2) → ARC-51**
    (physical layout APN — ASN(host) — ARC).

## Conventions

- Keep the connector interfaces as **KiCad 10 Design Blocks** where practical, instantiated
  here **and** in each board project, so the harness pinout stays in sync via the library
  link (mitigates the manual-mirror drift, since there is no cross-project linking).
- This artifact is **controller-scoped** and a living document: drafted once the architecture
  is firm, used as the connector contract during each panel's B2, finalized for B7/B8 harness
  build. It is **not** a per-panel B2 deliverable and has no fabrication output.

## Status

Scaffolded — block diagram not yet drawn.
