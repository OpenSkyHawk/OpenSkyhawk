# CAD Cutout Library (FreeCAD)

> **Status:** verified in FreeCAD 1.1.1, on a panel built with the
> [Cockpit Panels](https://github.com/OpenSkyHawk/CockpitPanels) workbench (Panel Blank →
> Plate Stack). CAD tooling (D8) is still open, so these are internal reference pages. Once
> D8 lands they get promoted into `docs/hardware/cad-workflow.md`.

Draw each control's cutout **once**, in a small library file, and link it into every panel
that uses it. Edit the library and every panel picks up the change.

Start the panel with the workbench: **Panel Blank** draws the outline and fastener grid,
centred on the origin, and **Plate Stack** builds one Part Design Body per layer
(`FacePlate`, `Diffuser`, `BackPlate` by default). The cutouts in these guides go into those
Bodies. See [CAD workflow → Panel base blank](../../hardware/cad-workflow.md#panel-base-blank)
for installing and using the workbench.

A panel places library cutouts in one of two ways. Both link the same library files, and
one panel can use both.

| Page | What it covers |
|---|---|
| [Library part](library-part.md) | Building a library file, its conventions, and how linked libraries behave (edits, renames, save prompts). Read this first for either method. |
| [Method A: LCS per control](lcs-placement.md) | One Local Coordinate System per control. Each control has its own position, rotation and KiCad reference. |
| [Method B: Point array](array-placement.md) | One Draft Point Array places a whole row or grid of identical, unrotated controls. |

## Pick a placement method

| | Method A: LCS per control | Method B: Point array |
|---|---|---|
| Rotation | Each control has its own | All copies share one orientation |
| Objects for N controls, 3 layers | N LCS + 3N binders + 3N pockets | 3 arrays + 6 binders + 3 pockets, whatever N is |
| Add a control | Add a point, an LCS, then a binder and pocket per layer | Add a point |
| Remove a control | Delete its objects, but **keep** its point (LCSs bind by index) | Delete the point |
| KiCad reference in the tree | Yes (the LCS label) | No, the points are anonymous |
| Parts per placement | Any part | One library part per array |
| Workbenches | Part Design | Part Design + Draft |

Use **Method A** when a control rotates, or when you want its KiCad reference visible in the
tree: toggles, rotaries with locating tabs, gauges. Use **Method B** for banks of identical,
unrotated controls: LED rows, annunciator lamps, push-button banks.

## Shared building blocks

- **Plate layers.** A panel is a stack of plate Bodies, one per Plate Stack layer:
  `FacePlate`, `Diffuser` and `BackPlate` by default. The library holds one sketch per
  layer, named exactly after the layer, because each plate needs a different hole.
- **Your own points sketch.** Panel Blank draws the outline and the fastener grid but no
  control positions. Make your own points sketch on the XY plane, outside the `Panel Blank`
  group, and dimension the points from the origin, which is the panel centre.
- **Sub-Shape Binder.** A live, cross-file copy of a library sketch. Both methods pocket a
  binder directly (Through all, Symmetric), so you don't need an External-Geometry cut
  sketch.
- **Cutouts go after the stack's own features.** Each layer Body already holds
  `<Layer>_Outline`, `<Layer>_Pad`, `<Layer>_Cut` and `<Layer>_Pocket` (the fastener cut).
  Add your binders and pockets after `<Layer>_Pocket`, and leave those four alone: running
  Plate Stack or Panel Blank again updates them in place.
- **Cutouts stay at the control points.** Plate Stack leaves every Body at the origin and
  sets each layer's depth on its outline binder instead, so a Through-all pocket cuts
  whatever the layer's depth. Don't move a plate Body's Placement: the plate slides while
  its holes stay where they were.
- **Cut files.** Export Layers flattens each plate to one DXF, drawn on a single layer named
  after the Body (`FacePlate.dxf` on layer `FacePlate`, …). Only features that go all the
  way through the plate appear, which the Through-all pockets here always do.

## Related

- [Mechanical standards](../../hardware/mechanical-standards.md): Dzus grid and mounting
  holes for the panel blank.
