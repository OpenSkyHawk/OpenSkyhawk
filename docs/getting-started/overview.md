---
status: new
---

# What is OpenSkyhawk?

OpenSkyhawk is an open source project to build a fully modular replica cockpit for the
DCS World **A-4E-C Skyhawk**. It covers the panel hardware, the PCBs behind it, the
firmware that ties it together, and — eventually — a replica stick, throttle, and rudder.

## The problem: a USB hub full of controllers

Build a home sim pit the usual way and you end up with a drawer full of separate USB
devices — a button box here, an encoder panel there, a third board for the gauges. Ten or
fifteen HID controllers, each with its own COM port, its own DCS bindings, its own profile
to maintain. Add a panel and you add another device to the pile. Windows only allows eight
HID joysticks before you start losing axes. The wiring sprawls, the bindings drift, and
every rebuild is an afternoon of re-mapping.

## The solution: one cable, one bus

**OpenSkyhawk replaces that pile with a single USB connection to the PC.**

Every panel, switch, knob, and gauge in the cockpit hangs off one shared **CAN bus** —
the same kind of robust, daisy-chained network a car uses to connect dozens of modules
over a single twisted pair. A small gateway board turns that bus into one tidy USB device
the PC sees as a single connection. Plug in one cable and the whole cockpit comes alive.

The result:

- **One USB connection** instead of 10–15 separate controllers
- **Add a panel by adding a node** on the bus — no new USB device, no PC-side rewiring
- **DCS-BIOS for everything that has an address** — switch positions and gauge values sync
  with the sim automatically, with zero manual binding
- **HID only where it's actually needed** — the flight stick, throttle, and rudder axes

If you want the full picture of how the three firmware tiers fit together, read
[How the System Works](system-overview.md).

## What you can build

The end goal is the complete A-4E-C cockpit: the center, left, and right consoles, the
instrument panel, lit panels and working gauges, and a replica HOTAS. Panels are organised
into **panel groups**, each driven by its own controller board on the CAN bus.

## Where the project is today

OpenSkyhawk is early and building in the open. Being honest about state:

- **Firmware backbone — working and hardware-verified.** The three-tier architecture
  (SimGateway, PanelBridge, PanelGroup), the CAN protocol, DCS-BIOS integration, and the
  first input and output types are implemented and tested on real hardware.
- **Armament Group — real hardware exists.** The Center Console Armament Group has a host
  board and two breakout boards with completed schematics. Its panel firmware is still a
  stub pending end-to-end integration.
- **Everything else — planned.** The rest of the consoles, the instruments, and the HOTAS
  are designed or scoped but not yet built.

!!! note "No kits for sale yet"
    The Armament Group is planned to be the first kit release, **pending successful
    end-to-end validation**. Nothing is for sale today. Follow the repository to track
    progress.

## Get involved

OpenSkyhawk serves three kinds of people. Pick the path that fits you.

**Build a panel** — want to fabricate hardware and wire up your own pit? Start with
[What You'll Need](prerequisites.md) and the [Repo Layout](repo-layout.md), then work
through [Your First Panel](../guides/first-panel.md).

**Contribute** — comfortable with firmware or PCB work? Read [How the System
Works](system-overview.md) and the [Design Decisions](../architecture/design-decisions.md)
first — they explain how it all fits together and why.

**Buy a kit** — want a kit you can assemble and fly? The Armament Group is slated to be the
first release. [Kit Assembly](../kits/index.md) docs are being written ahead of that.
