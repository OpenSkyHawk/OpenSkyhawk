# Method B: Point array

One **Draft Point Array** over a points sketch places every copy of a library cutout at
once. Each layer needs one array and one pocket, however many controls there are. Use this
method for rows or grids of identical controls that never rotate, such as indicator LEDs.
A control that rotates belongs in [Method A: LCS per control](lcs-placement.md). Back to
the [overview](index.md).

Build the [library part](library-part.md) first.

The screenshots come from a small demo: an `LED5` library part (FacePlate Ø5.2, BackPlate
Ø6.0) and a 100 × 30 mm panel with a row of five LEDs. The demo predates Plate Stack and has
only those two layers. On a Plate Stack panel, give the library a `Diffuser` sketch too and
repeat B2–B4 for the Diffuser Body.

## How the pieces fit

```
CAD/Shared/Cutouts/LED5.FCStd              ← library part
└── Body
    ├── FacePlate   (sketch: Ø5.2 hole)
    ├── Diffuser    (sketch: the Diffuser hole)
    └── BackPlate   (sketch: Ø6.0 hole)

<panel>.FCStd
├── Panel Blank (group, from Panel Blank)
├── Plate Stack (group, from Plate Stack)
├── LED_Points            (your sketch on XY: one point per LED, nothing else)
├── LEDs_FacePlate        (Draft PointArray over LED_Points)
│   └── LED5_FacePlate    (root binder → LED5 FacePlate sketch)
├── LEDs_Diffuser         (Draft PointArray over LED_Points)
│   └── LED5_Diffuser     (root binder → LED5 Diffuser sketch)
├── LEDs_BackPlate        (Draft PointArray over LED_Points)
│   └── LED5_BackPlate    (root binder → LED5 BackPlate sketch)
├── FacePlate (Body)
│   └── FacePlate_Outline … FacePlate_Pocket (from Plate Stack)
│       → LEDsCutFacePlate (Pocket) ← LEDsBinderFacePlate (binder → LEDs_FacePlate)
├── Diffuser (Body)
│   └── Diffuser_Outline … Diffuser_Pocket
│       → LEDsCutDiffuser (Pocket) ← LEDsBinderDiffuser (binder → LEDs_Diffuser)
└── BackPlate (Body)
    └── BackPlate_Outline … BackPlate_Pocket
        → LEDsCutBackPlate (Pocket) ← LEDsBinderBackPlate (binder → LEDs_BackPlate)
```

Each layer uses two binders:

- A **root binder** at the document root links the library sketch. The array copies it
  once per point.
- A **body binder** inside the plate Body links the array, and the pocket cuts it. You
  don't need a Placement expression, because the array already puts every copy in place.

## Recipe

### B1. Points sketch

1. In the panel, create a sketch on the **XY plane** (here `LED_Points`) with one
   **Point** per LED, dimensioned from the origin (the panel centre). Panel Blank doesn't
   make one for you; keep it outside the `Panel Blank` group.
2. Put **only** this array's points in it. The array places a copy on every vertex in the
   sketch, so a shared ControlPoints sketch would also put LEDs on your toggles.

![LED_Points: five points in a row](array-02-points.png)

### B2. Root binder (one per layer)

1. Make the panel the active document, with **no** body active, and select nothing.
2. Part Design → **Create a sub-object(s) shape binder**. With no active body, the binder
   lands at the document root, which is what this step needs.
3. In the tree, drag the library's layer sketch (`FacePlate`) onto the new binder. Leave
   its Placement alone.

![Library LED5: the FacePlate (Ø5.2) and BackPlate (Ø6.0) sketches to drag](array-01-library.png)

![Root binder LED5_FacePlate: Support points into the LED5 library](array-03-root-binder.png)

### B3. Point array (one per layer)

1. Switch to the Draft workbench. Select the root binder, then Ctrl-click `LED_Points`.
2. Modification → Array tools → **Point array**.
3. Check the array's properties: *Base* is the binder, *Point Object* is `LED_Points`, and
   *Count* matches the number of points. Rename the array (`LEDs_FacePlate`).

![Point array LEDs_FacePlate: Base LED5_FacePlate, Point Object LED_Points, Count 5](array-04-point-array.png)

### B4. Bind and cut

1. Back in Part Design, double-click the layer Body so it's active, then select nothing.
   Its Tip should be `<Layer>_Pocket` (or a later feature of yours), so the binder and
   pocket land after the stack's own features.
2. Create a sub-shape binder, then drag the **array** (`LEDs_FacePlate`) onto it.
3. With the binder selected, run **Pocket**: Mode **Symmetric**, Type **Through all**.

![Body binder LEDsBinderFacePlate: Support is the point array, claimed by the pocket](array-05-body-binder.png)

![Pocket on the array binder: Symmetric + Through all, all five holes previewed](array-06-pocket.png)

Repeat B2–B4 for the Diffuser and BackPlate layers, each with its own root binder on the
library's `Diffuser` or `BackPlate` sketch and its own array over the same `LED_Points`.
**Export Layers** then writes one DXF per layer Body, holes included.

![Result: five Ø5.2 holes in the FacePlate and five Ø6.0 holes in the BackPlate](array-07-result.png)

## Gotchas

- **One points sketch per array.** Every vertex becomes a copy, and that includes the
  endpoints of any normal line. Construction geometry is ignored. Keep each array's points
  in a sketch of its own.
- **All copies share one orientation.** The array's *Extra Placement* rotates every copy
  the same way. A control that needs its own angle belongs in Method A.
- **Deleting a point is safe here.** The array has no per-index links, so adding, moving
  or deleting points just changes the copies. This is the opposite of the Method A rule.
- **Which binder goes where.** The root binder (B2) sits at the document root, outside
  every plate Body, which is how this recipe was verified. The body binder (B4) sits
  inside the plate Body that it cuts. If either lands in the wrong place, drag it to the
  right container in the tree.
