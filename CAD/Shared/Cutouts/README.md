# Cutout Library

Reusable panel cutouts, one FreeCAD file per part (e.g. `Toggle12.FCStd`). Each file has
one Body with **one sketch per plate layer** (`FacePlate`, `BackPlate`), centred on the
origin. Panels link these sketches with Sub-Shape Binders and place each control with its
own LCS.

How to build and use them: [CAD Cutout Library](../../../docs/_source/cad-cutout-library.md).

- Edit sketches in place. Never delete and recreate one, because that breaks every panel
  that links to it.
- Don't move or rename a file once a panel links to it. Links use relative paths.
- `_spike/` holds the #335 test panel.
