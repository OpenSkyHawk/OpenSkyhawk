# Mechanical Standards

The mechanical conventions for panels and mounts. Some of this is settled (screws, gauge sizes);
panel-level dimensions are still being worked out and are marked TBD — don't invent them.

## Screws

| Screw | Use | Clearance |
|-------|-----|-----------|
| M2 | PCB mounts, small standoffs | — |
| M3 | Placards, light rings, small brackets | — |
| M4 | Instrument bezels, gauge mounts | Ø4.3–4.5 mm |
| M5 | Panel-to-subpanel, corner mounts | Ø5.3–5.5 mm |

## Gauges

| Gauge | Size | Drive |
|-------|------|-------|
| LOX gauge | 2-5/8″ (~67 mm) | — |
| Radar altimeter | 3-1/8″ (~100 mm with bezel) | — |
| Cabin pressure | — | X27.589 Switec stepper, shaft-through-PCB mount |

## Switches & controls

- Toggle switches: **12 mm** standard; **~6 mm** on the ECM modules.

## Panel dimensions, cutouts, bezels, light rings

!!! note "TBD — driven by the CAD models"
    Panel outline dimensions, switch/gauge cutout templates, bezel profiles, and light-ring
    geometry come from the Fusion/FreeCAD panel models, which are early-stage. These will be
    documented here as panels are modelled. CAD tooling is still under evaluation — see
    [CAD Workflow](cad-workflow.md).

## Board-level placement

Mechanical placement on the PCB itself — LEDs on the front face, everything else on the back,
through-hole connectors accessible from the panel side — is part of the
[PCB Design Rules](pcb-design-rules.md).
