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

### Dzus rail mounting — MIL-F-25173A

Left/right console panels mount on standard **Dzus rails** (MIL-F-25173A — see
[`docs/References/`](../References/index.md)). The width and mounting pattern are now settled:

| Dimension | Value |
|-----------|-------|
| **Panel width** — global default for L/R console panels | **146.05 mm** (5¾″) |
| **Mounting-stud centers**, left ↔ right | **136.5 mm** (5⅜″) — 4.76 mm (3⁄16″) inboard of each edge |
| **Vertical fastener pitch** (rail receptacle grid) | **9.525 mm** (3⁄8″); ≥ 2 studs/side, snapped to the grid |
| **Panel height** | N × 9.525 mm ("Dzus units"), per panel |
| **Panel thickness** | 1.59 mm (1⁄16″) aluminum |

**Fastener** — quarter-turn stud + receptacle strip:

- **Stud:** head Ø **.375″ (9.53 mm)** · body Ø **.257″ (6.53 mm)** · panel hole **~.26″ (6.6 mm)** ·
  .050″ screwdriver slot · 1⁄16″-panel grip · quarter-turn lock (85–135°). Detail design per
  MIL-F-25173A Fig 2 — **no single mandated stud P/N**; use any conforming quarter-turn stud
  (Dzus / Southco / Skybolt), chosen by head style + grip.
- **Receptacle strip (the rail):** Dzus **`PR 3½-L`** (or equal), cut to length · .051″ music-wire
  spring · aluminum · cadmium plated.

**Exceptions** (non-standard width — measured individually): throttle quadrant, lighting panel, and
any panel not on the standard Dzus rail.

### Per-panel cutouts / bezels / light rings

!!! note "TBD — driven by the CAD models"
    Switch/gauge cutout templates, bezel profiles, and light-ring geometry come from the
    Fusion/FreeCAD panel models, which are early-stage. These are documented per panel as they
    are modelled. CAD tooling is still under evaluation — see [CAD Workflow](cad-workflow.md).

## Board-level placement

Mechanical placement on the PCB itself — LEDs on the front face, everything else on the back,
through-hole connectors accessible from the panel side — is part of the
[PCB Design Rules](pcb-design-rules.md).
