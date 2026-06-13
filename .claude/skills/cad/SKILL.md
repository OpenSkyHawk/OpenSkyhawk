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

## Fit against the PCB

Export the board outline from KiCad (`kicad-cli pcb export step`) to check the panel model
against the actual PCB — see `docs/_source/kicad.md` for the CLI. Confirm screw clearances
against the hardware-standards screw table (M2 PCB mounts … M5 corner mounts).

> This skill is intentionally thin until D8 lands and the first panels are modeled — fill in
> the chosen-tool workflow then.
