---
status: planned
---

# Armament Group Kit — Assembly Guide

!!! warning "Planned kit — not yet available"
    This guide will be finalised with the kit. Steps and photos are **TBD** pending the first
    release. Don't follow it as a build recipe yet.

Assembling the Armament Group kit comes down to: prepare the boards, mount them in the printed
panel, and connect the harnesses. No soldering of fine-pitch ICs is expected at the buyer level —
the kit aims to be component-and-connector assembly.

## Steps (planned)

1. **Check your contents** against [What's in the Box](../whats-in-the-box.md).
2. **Mount the switches, knobs, and indicators** into the printed panel. *(photo TBD)*
3. **Fit the host board and the two breakout boards** (AWRS, Misc Switch). *(photo TBD)*
4. **Connect the signal harnesses** — JST-XH between the host board and each breakout. They're
   keyed; they only go in one way. *(photo TBD)*
5. **Connect power/bus** — the Molex Mini-Fit Jr connectors carry 12 V / 5 V and CAN to the rest
   of the cockpit. *(photo TBD)*
6. **Double-check orientation** on the keyed connectors before powering anything.

!!! note "First panel group in your cockpit?"
    The Armament Group is a CAN node — it needs the shared **SimGateway** and **PanelBridge**
    backbone to reach the PC. If this is your first OpenSkyhawk panel, you'll set those up once;
    every later panel group reuses them.

Next: [Flashing Firmware](flashing.md).
