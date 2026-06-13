---
status: planned
---

# Flight Controls

This section covers the replica **out-of-console** flight hardware — the standalone units that
sit outside the cockpit panels: the control **stick** and the **rudder pedals**. These are
their own physical builds, separate from the console panels.

!!! note "Throttle: panel vs. axis — two different things"
    The **throttle panel** (the quadrant with its switches and indicators) is a
    [Left Console](../panels/left-console/index.md) panel and goes through DCS-BIOS. The
    **throttle axis** (analog lever position, `CTRL_THROTTLE`) is one of the HID axes below.
    Don't conflate them.

## HID axes

The replica flight controls connect to the PC over the HID path (not DCS-BIOS), because the
A-4E-C exposes no axis exports through DCS-BIOS. The planned axis allocation uses 7 of the 8
DirectInput axis slots:

| Axis | CTRL constant | Control |
|------|---------------|---------|
| 0 | `CTRL_ROLL` | Roll — replica stick |
| 1 | `CTRL_PITCH` | Pitch — replica stick |
| 2 | `CTRL_THROTTLE` | Throttle position — replica throttle |
| 3 | `CTRL_RUDDER` | Rudder — rudder pedals |
| 4 | `CTRL_BRAKE_L` | Left toe brake — rudder pedals |
| 5 | `CTRL_BRAKE_R` | Right toe brake — rudder pedals |
| 6 | `CTRL_ZOOM` | Zoom |

The **speed brake** is a DCS-BIOS on/off switch, not a HID axis.

See [DCS-BIOS vs HID](../architecture/dcsbios-vs-hid.md) for why these go through HID.

## Status

!!! note "All planned — no hardware yet"
    Nothing here is designed yet. These will be **CAD-heavy** builds once started — mostly
    printed and machined mechanical parts. CAD tooling is currently under evaluation
    (Fusion 360 vs FreeCAD); see [Design Decisions D8](../architecture/design-decisions.md).

## Interested in contributing to the HOTAS build?

The stick and pedals are a big, self-contained chunk of mechanical and firmware work. If
that's your thing, start with [How to Contribute](../contributing/index.md).
