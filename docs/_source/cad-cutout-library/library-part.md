# Library part

A library part is one FreeCAD file per control type, kept in `CAD/Shared/Cutouts/`. Both
placement methods ([LCS per control](lcs-placement.md) and [point array](array-placement.md))
link these files. Back to the [overview](index.md).

```
CAD/Shared/Cutouts/Toggle12.FCStd
└── Body
    ├── FacePlate   (sketch: Ø12.6 hole)
    └── BackPlate   (sketch: Ø12.6 hole + anti-rotation tab)
```

The library holds one sketch per **plate layer**, because the plates need different holes.
In Toggle12, for example, the BackPlate layer carries the anti-rotation tab and the
FacePlate layer doesn't.

## Conventions

| Rule | Why |
|---|---|
| One file per part, one Body, one sketch per plate layer | Binders link to a sketch by name, so each layer must be its own sketch |
| Label sketches by layer: `FacePlate`, `BackPlate` | Tells you which binder points where |
| Centre the cutout on the sketch origin (0,0) | Both methods place the origin, so (0,0) is the control's centre |
| Edit sketches in place: never delete and recreate them | The binder links to the sketch's internal name. A recreated sketch breaks every panel |
| Save the library in its final folder **before** linking it | Links store a relative path. Moving or renaming the file later breaks them |

## Recipe

1. New file, saved in `CAD/Shared/Cutouts/` (e.g. `Toggle12.FCStd`).
2. Part Design → Create body, then one sketch per layer on the XY plane.
3. Draw each layer's cutout, centred on the origin. Rename the sketches `FacePlate` and
   `BackPlate`.
4. Save.

![Library BackPlate sketch: Ø12.6 hole plus anti-rotation tab, centred on the origin](01-library-sketch.png)

## How linked libraries behave

- **The library doesn't need to be open.** Opening a panel loads its linked libraries
  automatically.
- **Library edits show up on recompute.** After you edit and save a library sketch, the
  panel's binders are marked as changed the next time the panel opens. Press Ctrl+R to
  update the cutouts.
- **"Save dependent files?"** When you save a panel, this prompt means a linked library has
  unsaved edits. Answer **No** unless you changed the library on purpose.
- **A renamed or moved library fails quietly.** The panel still opens and the cutouts keep
  their last shape. The tree shows no error. The only sign is a red `Link not restored`
  line in the **Report view**. Put the file back under its old name and the link returns,
  even if you saved the panel while the link was broken. Check the Report view after any
  rename or move in `CAD/Shared/Cutouts/`.

  ![Renamed library: no error in the tree, "Link not restored" in the Report view](09-rename-broken.png)
