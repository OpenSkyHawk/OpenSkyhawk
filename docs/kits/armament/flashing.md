---
status: planned
---

# Armament Group Kit — Flashing Firmware

!!! warning "Planned kit — not yet available"
    Final flashing instructions ship with the kit. Details below are **TBD**.

The kit's controller runs OpenSkyhawk firmware. Whether boards arrive **pre-flashed** or you
flash them yourself is still TBD for the first release — likely pre-flashed so the kit is
assemble-and-go.

## If you need to flash it yourself (planned)

1. Install the firmware toolchain (PlatformIO) — see [What You'll Need](../../getting-started/prerequisites.md).
2. Open the Armament Group firmware project.
3. Flash the controller over its SWD header with an ST-Link.

The detailed mechanics (ST-Link, `pio run -t upload`, clone quirks) are in the builder guide:
[Flashing Firmware](../../guides/flashing.md). Kit buyers shouldn't normally need it.

!!! note "NODE_ID is already set"
    The Armament Group is **NODE_ID 1**. The kit firmware is built for it — you don't choose or
    change it.

Next: [DCS Setup](dcs-setup.md).
