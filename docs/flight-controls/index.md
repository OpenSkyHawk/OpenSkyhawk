---
status: planned
---

# Flight Controls

This section covers the replica **out-of-console** flight hardware — the standalone units that
sit outside the cockpit panels: the control **stick** and the **rudder pedals**. These are
their own physical builds, separate from the console panels.

!!! note "The throttle is a Right Console panel"
    The throttle lever is **not** part of this section — it lives on the
    [Right Console](../panels/right-console/index.md) as a console panel. This section is the
    stick and rudder pedals only.

## HID axes

The replica flight controls connect to the PC over the HID path (not DCS-BIOS), because the
A-4E-C exposes no axis exports through DCS-BIOS. The planned axis allocation uses 7 of the 8
DirectInput axis slots:

| Axis | Control |
|------|---------|
| Roll | Stick |
| Pitch | Stick |
| Rudder | Pedals |
| Speed Brake | Throttle (Right Console) |
| Left Brake | Pedals |
| Throttle | Throttle (Right Console) |
| Zoom | — |

See [DCS-BIOS vs HID](../architecture/dcsbios-vs-hid.md) for why these go through HID.

## Status

!!! note "All planned — no hardware yet"
    Nothing here is designed yet. These will be **CAD-heavy** builds once started — mostly
    printed and machined mechanical parts. CAD tooling is currently under evaluation
    (Fusion 360 vs FreeCAD); see [Design Decisions D8](../architecture/design-decisions.md).

## Interested in contributing to the HOTAS build?

The stick and pedals are a big, self-contained chunk of mechanical and firmware work. If
that's your thing, start with [How to Contribute](../contributing/index.md).
