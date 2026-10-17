---
name: cad
description: OpenSkyhawk CAD / mechanical work — panel and bezel 3D models, enclosures, mechanical fit. Use when modeling a panel in Fusion 360 or FreeCAD, working with .f3d/.FCStd source, exporting STL/STEP, preparing parts for 3D printing, or checking mechanical fit against a PCB. Consumes panel dimensions from the panel-mapping skill.
---

# CAD / Mechanical

3D models for panels, bezels, and enclosures. The panel's real-world dimensions and
control positions come from the `panel-mapping` skill; this skill turns them into a model.

## Tooling decision is pending (D8)

**CAD tooling is under evaluation — Fusion 360 vs FreeCAD, with FreeCAD currently preferred.**
Do not present either as the settled choice; see `docs/architecture/design-decisions.md` (D8)
and `docs/hardware/cad-workflow.md`. Update this skill once D8 is resolved.

- Source files live in `CAD/<Console>/...` as `.f3d` (Fusion) or `.FCStd` (FreeCAD).
- **Commit source only.** STL and STEP exports are gitignored — generate them from source;
  publish built artifacts via Releases, not the repo.
- A Fusion MCP server is available (read/execute/update Fusion designs directly) if working
  in Fusion.

## Inputs from panel-mapping

- panel outline dimensions: real-world mm = `√(ΔX²+ΔY²+ΔZ²) × 1.10 × 1000` from Model Viewer
  corner coordinates (1 model unit = 1000 mm, × 1.10 scale).
- switch centre-to-centre spacings (measured in CAD once the panel body exists).
- the control inventory (what cutouts/mounts the panel needs).

## Panel blank + plate stack (CockpitPanels workbench)

Panel bases come from the **Cockpit Panels** FreeCAD 1.1+ workbench, a separate repo:
`OpenSkyHawk/CockpitPanels` (LGPL-2.1+; install via Addon Manager → custom repository,
branch `main`). Its README is the reference. Three tools, in order:

- **Panel Blank**: Dzus-grid outline + mount points + clearance holes, centred on the origin
  on XY, driven by the `PanelBlank` sheet. No ControlPoints sketch: the user makes their own.
- **Plate Stack**: one Body per layer (default `FacePlate` 1/16″, `Diffuser` 1/8″,
  `BackPlate` 1/8″), front face at z = 0, each with `<Layer>_Outline/_Pad/_Cut/_Pocket` and a
  per-layer fastener cut (Clearance / Recess / Slot / Slot (open ends)); faceplate insets.
- **Export Layers**: one DXF per plate Body, on a layer named after the Body.

Published overview: `docs/hardware/cad-workflow.md` → *Panel base blank*.

## Cutout library

Reusable panel cutouts live in `CAD/Shared/Cutouts/` and are linked into panels with
Sub-Shape Binders, then pocketed inside the Plate Stack layer Bodies, after each layer's
`<Layer>_Pocket`. Library parts carry one sketch per layer, named exactly like the layers
(`FacePlate`, `Diffuser`, `BackPlate`). Docs: `docs/_source/cad-cutout-library/`. Start at
`index.md` (method choice), then `library-part.md` plus one method page:

- `lcs-placement.md`: ControlPoints → one LCS per control → binder per plate layer →
  Pocket. Key gotcha: LCS "Translate origin" ignores the offset rotation, so the binder
  Placement expression is `placement(SWn.Placement.Base; SWn.AttachmentOffset.Rotation)`.
- `array-placement.md`: for rows of identical, unrotated controls (LEDs), a Draft Point
  Array over a dedicated points sketch replaces the per-control LCS and binders.

## Fit against the PCB

Export the board outline from KiCad (`kicad-cli pcb export step`) to check the panel model
against the actual PCB — see `docs/_source/kicad.md` for the CLI. Confirm screw clearances
against the hardware-standards screw table (M2 PCB mounts … M5 corner mounts).

> This skill is intentionally thin until D8 lands and the first panels are modeled — fill in
> the chosen-tool workflow then.
