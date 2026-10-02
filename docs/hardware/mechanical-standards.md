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
| **Mounting-stud centers**, left ↔ right | **136.525 mm** (5⅜″) — 4.7625 mm (3⁄16″) inboard of each edge |
| **Vertical fastener pitch** (rail receptacle grid) | **9.525 mm** (3⁄8″); ≥ 2 studs/side |
| **Panel height** | N × 9.525 mm ("Dzus units"), per panel |
| **Stud rows**, from the panel's own top edge | outermost at **1.5 × pitch**; every row at a **half-integer** multiple |
| **Panel thickness** | 1.59 mm (1⁄16″) aluminum |

Work from the exact inch fractions, not the rounded millimetres: the inset is exactly half a
pitch, so the stud span is `panel width − pitch` = 146.05 − 9.525 = **136.525 mm**. Rounding the
inset to 4.76 does not close against a 146.05 panel.

### Where the stud rows go

The outermost stud sits **1.5 pitch units** from each end of the panel (`e = 1.5`) — the
convention these panels are built to, verified against the A-4's Doppler panel, which has an
independent height. So a panel of N units carries its outer studs `(N − 3) × 9.525 mm` apart.

Because `e` is a half-integer and panel heights are whole units, **every stud in a stack lands at
`(k + 0.5) × 9.525 mm` from the top of the stack**. Any additional row must therefore also be at a
half-integer position (2.5, 4.5, 6.5 …); a row at a whole unit misses the rail by half a pitch.
Across a joint the spacing is always `(N − 1.5) → (N + 1.5)` = **3 pitch = 28.575 mm**, whatever
the two panels' heights, and panels butt with no gap.

Worked example — a 6-unit panel (57.15 mm): rows at 1.5 and 4.5, i.e. **14.2875** and
**42.8625 mm** from the top edge, both rails at x = **4.7625** and **141.2875 mm**. That is the
`136.525 × 28.575 mm` pattern the APN-153 panel uses.

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
