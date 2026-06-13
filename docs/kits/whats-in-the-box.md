---
status: planned
---

# What's in the Box

!!! warning "Planned kit — not yet available"
    This describes the **planned** Armament Group kit. Contents are not final and the kit is not
    yet for sale — it's pending end-to-end validation. Final bill of materials and quantities
    are **TBD**.

The Armament Group kit is planned to cover the Center Console armament panels on a single CAN
node (**NODE_ID 1**).

## Planned contents

| Item | Notes |
|------|-------|
| Armament Panel host board | The STM32 MCU board (the panel group controller) |
| AWRS breakout board | Analog inputs via ADS1115 |
| Misc Switch breakout board | Digital I/O via MCP23017 |
| Panel hardware | Switches, knobs, indicators *(final list TBD)* |
| Harnesses | JST-XH signal + Molex Mini-Fit Jr power/bus, pre-crimped *(TBD)* |
| Printed panel parts | Enclosure, bezels, knobs *(TBD — CAD in progress)* |
| Fasteners | M2–M5 per the build *(TBD)* |

## You'll also need (not in the kit)

- A PC running **DCS World** with the free **A-4E-C** community module
- The shared cockpit backbone — a **SimGateway** (RP2040) and a **PanelBridge** (STM32) — if
  this is your first panel group. (These are one-per-cockpit; later panel groups reuse them.)
- A USB cable to the PC

!!! note "Photos TBD"
    An unboxing photo and a labelled parts-layout image will be added here once the kit is
    finalised. **(photo TBD)**

When your contents are confirmed, continue to the [Assembly Guide](armament/assembly.md).
