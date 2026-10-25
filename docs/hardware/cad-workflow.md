---
status: stub
---

# CAD Workflow

!!! warning "Work in Progress"
    This page is being written. Check back soon or
    [contribute to the docs](../contributing/index.md).

!!! note "CAD Tooling Under Evaluation"
    CAD tooling is currently being evaluated between Fusion 360 and FreeCAD.
    FreeCAD is the preferred direction for alignment with the open source nature
    of the project. This page will be updated once a tool is selected.
    The file management policy (source files committed, STL/STEP exports
    gitignored and published via GitHub Releases) applies regardless of tool choice.

## Panel base blank

Every side-console panel starts from the same base: a 5¾″-wide outline, a whole number of
⅜″ Dzus units tall, with fasteners on the rail grid (see
[Dzus rail mounting](mechanical-standards.md#dzus-rail-mounting-mil-f-25173a)). You don't
draw that base by hand. The **Cockpit Panels** workbench for FreeCAD 1.1+ builds it, then
turns it into the plates you cut. It lives in its own repository,
[OpenSkyHawk/CockpitPanels](https://github.com/OpenSkyHawk/CockpitPanels), because the
panel grid is common to many aircraft, not only the A-4.

Install it through FreeCAD's Addon Manager: add `https://github.com/OpenSkyHawk/CockpitPanels`
(branch `main`) as a custom repository in the Addon Manager preferences, then search the list
for **CockpitPanels**. The
[CockpitPanels README](https://github.com/OpenSkyHawk/CockpitPanels#install) has the
step-by-step install. Everything the workbench creates is a plain FreeCAD object, so a panel file
opens and recomputes on any FreeCAD 1.1+, with or without the workbench installed.

The workbench has three tools, used in order. Their buttons sit on the Cockpit Panels
toolbar and also on the Sketcher and Part Design toolbars.

**Panel Blank** draws the base, centred on the origin on the XY plane: the outline, a mount
point at every fastener position and a clearance hole at each one. Give it the height in
units (or a measured height, which it rounds to the nearest unit), any extra fastener rows,
the fastener and an optional corner radius. All of it is driven by a small spreadsheet, so
editing a cell moves everything. Running Panel Blank again updates the same objects in
place, and anything built on them keeps working.

**Plate Stack** turns the blank into one Part Design Body per layer. The default stack is a
1/16″ FacePlate, a 1/8″ Diffuser and a 1/8″ BackPlate, and you can rename, add or remove
layers and change their thicknesses. The faceplate's front face sits at z = 0, and each
layer sits directly behind the one before, so the Top view shows the panel as it looks in
the cockpit. Each layer gets its own fastener cut: a plain clearance hole, a recess that
lets the screw head sit down in the layer, or a slot that opens through the side edge (and
optionally through the top and bottom edges). The faceplate can also be inset from the
panel's edges so it sits back from the plates behind it.

**Export Layers** writes the laser-cut files: one DXF per plate, named after the plate's
Body and drawn on a layer of the same name. Each file holds the plate's outline and every
hole that goes all the way through it.

Control cutouts go into the plate Bodies after Plate Stack has built them. Draw each
control's cutout once, in a shared library part, and link it into every panel that uses it.
