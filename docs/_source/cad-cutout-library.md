# CAD Cutout Library (FreeCAD)

> **Status:** spike for #335, verified in FreeCAD 1.1.1. CAD tooling (D8) is still open,
> so this is an internal reference page. Once D8 lands it gets promoted into
> `docs/hardware/cad-workflow.md`.

Draw each control's cutout **once**, in a small library file, and link it into every panel
that uses it. Edit the library and every panel picks up the change. Each control in a
panel can still be moved and rotated on its own.

The screenshots below come from a small demo built for this page: a Toggle12-style library
part and a 100 × 50 mm BackPlate with two controls, SW1 and SW2. SW2 is rotated 90°.

## How the pieces fit

```
CAD/Shared/Cutouts/Toggle12.FCStd          ← library part (one file per part)
└── Body
    ├── FacePlate   (sketch: Ø12.6 hole)
    └── BackPlate   (sketch: Ø12.6 hole + anti-rotation tab)

<panel>.FCStd                              ← the panel
├── Panel Blank (group)
│   └── ControlPoints   (sketch: one point per control)
├── Controls (group)
│   ├── SW1   (LCS on ControlPoints:Vertex1)
│   └── SW2   (LCS on ControlPoints:Vertex2, rotated 90°)
├── FrontPlate (Body)
│   └── … SW1_Face (binder → FacePlate) → Pocket, SW2_Face → Pocket
└── BackPlate (Body)
    └── … SW1_Back (binder → BackPlate) → Pocket, SW2_Back → Pocket
```

- **ControlPoints**: the *where*. One plain sketch point per control. Move a point and the
  control moves on every plate.
- **One LCS per control**: the *which way*. A Local Coordinate System is pinned to its
  point and carries that control's rotation. The LCS label is the KiCad reference (`SW1`).
- **One Sub-Shape Binder per control per plate layer**: a live, cross-file copy of the
  library sketch for that layer, placed by the control's LCS.
- **Pocket the binder directly** (Through all, Symmetric). You don't need an
  External-Geometry cut sketch.

The library holds one sketch per **layer**, because the plates need different holes. In
Toggle12, for example, the BackPlate layer carries the anti-rotation tab and the FacePlate
layer doesn't.

## Library conventions

| Rule | Why |
|---|---|
| One file per part, one Body, one sketch per plate layer | Binders link to a sketch by name, so each layer must be its own sketch |
| Label sketches by layer: `FacePlate`, `BackPlate` | Tells you which binder points where |
| Centre the cutout on the sketch origin (0,0) | The LCS places the origin, so (0,0) is the control's centre |
| Edit sketches in place: never delete and recreate them | The binder links to the sketch's internal name. A recreated sketch breaks every panel |
| Save the library in its final folder **before** linking it | Links store a relative path. Moving the file later breaks them |

## Panel conventions

| Rule | Why |
|---|---|
| ControlPoints uses ordinary (non-construction) points | The LCS attaches to `Sketch:VertexN` |
| Never delete and re-add points in ControlPoints. Move them instead | The LCS binds by vertex **index**, so deleting a point renumbers the rest |
| LCS label = KiCad reference (`SW1`, `S3`, …) | Ties CAD to schematic and firmware |
| Keep the LCSs in a `Controls` group | Keeps the tree tidy when there are dozens |
| Save the panel in its final folder **before** creating binders | Same relative-path reason as the library |

## Recipe

### 1. Library part

1. New file, saved in `CAD/Shared/Cutouts/` (e.g. `Toggle12.FCStd`).
2. Part Design → Create body, then one sketch per layer on the XY plane.
3. Draw each layer's cutout, centred on the origin. Rename the sketches `FacePlate` and
   `BackPlate`.
4. Save.

![Library BackPlate sketch: Ø12.6 hole plus anti-rotation tab, centred on the origin](cad-cutout-library/01-library-sketch.png)

### 2. Control positions

1. In the panel, open the `ControlPoints` sketch (Sketcher workbench).
2. Add one **Point** per control and dimension it from the origin.
3. Close the sketch.

![ControlPoints sketch: one point per control, dimensioned from the origin](cad-cutout-library/02-control-points.png)

### 3. One LCS per control

1. Select nothing. Then Part (or Part Design) → **Create datum → Coordinate system**.
2. In the attachment panel, **type** the reference into the field: `Sketch:Vertex1`
   (use `ControlPoints`' internal name, shown in the tooltip; it's usually `Sketch`).
3. Attachment mode: **Translate origin**. OK.
4. Rename it to the KiCad reference (`SW1`) and drag it into the `Controls` group.
5. **To rotate the control:** edit SW1 again (double-click it) and set **Around Z-axis**
   under *Attachment Offset*.

![Attachment panel for SW1: reference Sketch:Vertex1, mode Translate origin](cad-cutout-library/03-lcs-attach.png)

![SW2's attachment offset: Around Z-axis = 90°](cad-cutout-library/04-lcs-rotation.png)

### 4. Bind and cut (per control, per plate)

1. Make the **panel** the active document. Double-click the plate Body (e.g. FrontPlate)
   so it's active, and select **nothing**.
2. Part Design → **Create a sub-object(s) shape binder**. An empty binder appears inside the
   body.
3. In the tree, **drag** the library's layer sketch (`FacePlate` or `BackPlate`) onto
   the new binder. Both documents appear in the same tree.

   ![Empty binder in the plate body, with the library sketch to drag](cad-cutout-library/05-empty-binder.png)

4. Select the binder → Property view → right-click **Placement** → **Expression…** and
   enter:

   ```
   placement(SW1.Placement.Base; SW1.AttachmentOffset.Rotation)
   ```

   Use the LCS's internal **Name**, which may be `LCS` or `LCS001` and not the `SW1`
   label. When it works, *Support* shows the library sketch and *Placement* turns blue,
   which means it's driven by the expression.

   ![Linked binder: Support points into the library file, Placement is expression-driven](cad-cutout-library/06-binder-linked.png)
5. With the binder selected, run **Pocket**: set Mode to **Symmetric** and Type to
   **Through all**. OK.

   ![Pocket on the binder: Symmetric + Through all](cad-cutout-library/07-pocket.png)
6. Rename the binder `SW1_Face` (or `SW1_Back` on the BackPlate).
7. Save. When you're asked "Save dependent files?", answer **No** unless you edited the
   library on purpose.

Repeat for the other plate layer. In the result below, SW2 is rotated 90°: its tab sits to
the side, while SW1's points down.

![Result: SW1 at 0°, SW2 at 90°; SW2 selected, showing its Translate attachment and 90° offset](cad-cutout-library/08-result.png)

## Gotchas

- **Translate origin ignores the offset rotation.** This is the big one. The LCS applies
  the Attachment Offset *translation* but drops its *rotation*. Vertex-only references
  support no other attachment mode. So the binder expression takes the position from the
  LCS and the rotation from the offset:
  `placement(SWn.Placement.Base; SWn.AttachmentOffset.Rotation)`. Plain `SWn.Placement`
  never rotates.
- **The binder has no Attachment.** A Sub-Shape Binder can't be attached in 1.1.1, so the
  Placement expression is how it gets positioned.
- **The binder lands in the wrong file or at the document root.** That happens when the
  library document was active or no body was active. Delete it and redo step 4.1 with the
  panel active and the body double-clicked. If it appears at the root, drag it into the
  body.
- **You can't pick a sketch point.** Points are drawn tiny and the plates cover Z=0.
  Raise *Preferences → Display → 3D View → Pick radius* to about 10 px, or just type
  `Sketch:VertexN` into the attachment field.
- **Clicks become references.** While the attachment panel shows "Selecting…", every
  click in the tree or the 3D view is added as a reference. A bad click leaves the LCS
  invalid (red `!`). Re-open it and re-type the reference.
- **Don't move the body, stack plates with the Pad.** The binder compensates for its
  container's placement, so moving the Body leaves the cutouts behind. Set the plate's
  Z position on the Pad (Reversed) and leave the Body placement alone.
- **`=` doesn't open the expression editor.** It only works while a value is being edited.
  Use right-click → **Expression…**.
- **"Save dependent files?"** means the linked library has unsaved edits. Answer **No**
  unless you changed the library deliberately.
- **No live rotation preview.** The cutout updates when you recompute (Ctrl+R) after you
  change the angle.
- **The library doesn't need to be open.** Opening the panel loads the linked library
  automatically.

## Verification (#335 spike)

| Check | Result |
|---|---|
| Move a ControlPoints point → both layers follow | ✅ |
| Rotate an LCS → both layers rotate | ✅ |
| Pocket a cross-file binder directly | ✅ |
| Opening the panel alone auto-loads the library | ✅ |
| Edit a library sketch → panel cutouts update | ⬜ |
| Rename or move the library → link breaks visibly (not silently) | ⬜ |

## Future: array placement for unrotated controls

One LCS per control gives each control its own rotation. For a row or grid of identical
controls that never rotate, a single array driven by the ControlPoints sketch could replace
the per-control LCS and binder. That approach isn't prototyped yet.

## Related

- #334: the blank-panel generator should also emit `ControlPoints` and one LCS per
  control, and wire the binders with the rotation-aware expression above.
- [Mechanical standards](../hardware/mechanical-standards.md): Dzus grid and mounting
  holes for the panel blank.
- The spike files, `CAD/Shared/Cutouts/Toggle12.FCStd` and
  `CAD/Shared/Cutouts/_spike/PanelBlank-Test.FCStd`, land with the #335 CAD branch.
