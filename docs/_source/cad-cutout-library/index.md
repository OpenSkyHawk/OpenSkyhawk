# CAD Cutout Library (FreeCAD)

> **Status:** verified in FreeCAD 1.1.1. CAD tooling (D8) is still open, so these are
> internal reference pages. Once D8 lands they get promoted into
> `docs/hardware/cad-workflow.md`.

Draw each control's cutout **once**, in a small library file, and link it into every panel
that uses it. Edit the library and every panel picks up the change.

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
| Objects for N controls, 2 layers | N LCS + 2N binders + 2N pockets | 2 arrays + 4 binders + 2 pockets, whatever N is |
| Add a control | Add a point, an LCS, then a binder and pocket per layer | Add a point |
| Remove a control | Delete its objects, but **keep** its point (LCSs bind by index) | Delete the point |
| KiCad reference in the tree | Yes (the LCS label) | No, the points are anonymous |
| Parts per placement | Any part | One library part per array |
| Workbenches | Part Design | Part Design + Draft |

Use **Method A** when a control rotates, or when you want its KiCad reference visible in the
tree: toggles, rotaries with locating tabs, gauges. Use **Method B** for banks of identical,
unrotated controls: LED rows, annunciator lamps, push-button banks.

## Shared building blocks

- **Plate layers.** A panel is a stack of plate Bodies (FacePlate, BackPlate, …). The
  library holds one sketch per layer, because each plate needs a different hole.
- **Sub-Shape Binder.** A live, cross-file copy of a library sketch. Both methods pocket a
  binder directly (Through all, Symmetric), so you don't need an External-Geometry cut
  sketch.
- **Cutouts stay at the control points.** A binder inside a Body compensates for that
  Body's Placement. You can stack plates by moving a Body in Z, and the Through-all pocket
  still cuts. Don't move a plate Body in X or Y: the plate slides while its holes stay
  where they were.

## Related

- [Mechanical standards](../../hardware/mechanical-standards.md): Dzus grid and mounting
  holes for the panel blank.
