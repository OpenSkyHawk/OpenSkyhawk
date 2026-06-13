---
status: planned
---

# Armament Group Kit — DCS Setup

!!! warning "Planned kit — not yet available"
    Final setup steps ship with the kit. Details below are **TBD** pending validation.

This is where the OpenSkyhawk approach pays off: the Armament Group's controls are **DCS-BIOS**
controls, so there's **no per-switch binding** to do in DCS. Run the export, plug in, fly.

## Steps (planned)

1. **Install the A-4E-C module** — the free A-4E-C Skyhawk community mod for DCS World.
2. **Install and run DCS-BIOS** — the export bridge between DCS and your cockpit. This is the
   one piece of PC-side setup.
3. **Plug in the cockpit's USB cable** — the single connection from the SimGateway to the PC.
4. **Start a mission** in the A-4E-C. Your panel's switches and indicators sync automatically.

!!! note "No joystick binding for armament controls"
    Because the armament panels use the DCS-BIOS path, you do **not** open DCS controls settings
    and map switches. DCS-BIOS handles state both ways. (HID binding only matters for the
    out-of-console flight controls — stick, rudder pedals — not this kit.)

## What "working" looks like

Flip a switch on the panel and watch it move in the cockpit; trigger a sim state (e.g. a caution
condition) and watch the panel indicator follow. If it doesn't, see
[Testing & Troubleshooting](testing.md).
