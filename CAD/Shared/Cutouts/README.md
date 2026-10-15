# Cutout Library

Reusable panel cutouts, one FreeCAD file per part (e.g. `Toggle12.FCStd`). Each file has
one Body with **one sketch per plate layer** (`FacePlate`, `BackPlate`), centred on the
origin. Panels link these sketches with Sub-Shape Binders. Each control gets its own LCS,
or a whole row of identical, unrotated controls (such as LEDs) is placed with one Draft
Point Array.

How to build and use them: [CAD Cutout Library](../../../docs/_source/cad-cutout-library/index.md)
(building a [library part](../../../docs/_source/cad-cutout-library/library-part.md) is
covered on its own page).

- Edit sketches in place. Never delete and recreate one, because that breaks every panel
  that links to it.
- Don't move or rename a file once a panel links to it. Links use relative paths, and a
  broken link only shows up as `Link not restored` in the Report view.