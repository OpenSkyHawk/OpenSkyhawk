---
status: planned
---

# Kit Assembly

OpenSkyhawk plans to sell **panel and PCB kits** so you can build a cockpit without sourcing
parts or laying out boards yourself. This section is the assembly track for kit buyers — it
stands on its own. You won't need to read PCB design rules or firmware internals to put a kit
together and fly it.

!!! warning "No kits are available yet"
    The **Armament Group** is the planned **first kit release**, pending successful end-to-end
    validation. Nothing is for sale today. Follow the
    [repository](https://github.com/OpenSkyhawk/OpenSkyhawk) to track progress.

## The kit-buyer journey

When the Armament Group kit ships, getting it working is four steps — each its own page:

1. **[What's in the Box](whats-in-the-box.md)** — check your kit contents
2. **[Assembly Guide](armament/assembly.md)** — put the boards and panel together
3. **[Flashing Firmware](armament/flashing.md)** — load the firmware
4. **[DCS Setup](armament/dcs-setup.md)** — connect to the sim
5. **[Testing & Troubleshooting](armament/testing.md)** — verify everything works

## What makes this different

The whole point of OpenSkyhawk: **one USB cable** for the entire cockpit, and **DCS-BIOS does
the binding for you** — no per-switch mapping in DCS. A kit is meant to be assemble-flash-fly,
not an electronics project.

## Want to help get kits shipped?

Kits are gated on end-to-end validation of the Armament Group firmware. If you want to help get
there, see [How to Contribute](../contributing/index.md).
