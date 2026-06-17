# Adding a New Panel Group

A panel group is one STM32 CAN node serving one area of the cockpit. This is the step-by-step
for adding one. The [Adding a Controller](../contributing/adding-controller.md) page is the
overview; this is the recipe.

## Step 1 — Claim a NODE_ID

**Always first.** Open `Firmware/NODE_IDS.md`, take the next free number (currently **2**), and
add your row (panel group, console, status). Commit it in the same PR that starts the work.
NODE_IDs are permanent — never reuse one. See [NODE_ID & CAN Addressing](../firmware/node-id.md).

## Step 2 — Create the firmware project

Copy `Firmware/Templates/PanelGroup/` to `Firmware/Panels/<YourPanel>/`. In `platformio.ini`:

- Set `-DNODE_ID=N` to the number you just claimed.
- Set `board` per the [variant policy](../firmware/platformio-setup.md) — `genericSTM32F103C8`
  is the default for every board.

Don't copy an existing panel's `platformio.ini` — use the template.

## Step 3 — Write the wiring map and controls

At the top of `main.cpp`, define a **wiring map** — one named `PinRef` per physical connection,
matching your schematic net labels:

```cpp
const PinRef PIN_MASTER_ARM(PB5);
const PinRef PIN_GEAR_LED(PB0);
```

Then declare control objects, choosing each control's routing — DCS-BIOS (`DCSIN_*`) or HID
(`CTRL_*`). The decision rule is on [DCS-BIOS vs HID](../architecture/dcsbios-vs-hid.md):

```cpp
OpenSkyhawk::Switch2Pos masterArm(DCSIN_ARM_MASTER, PIN_MASTER_ARM);
OpenSkyhawk::LED        gearLed  (A_4E_C_GEAR_LIGHT, A_4E_C_GEAR_LIGHT_AM, PIN_GEAR_LED);
```

`setup()` calls `PanelGroup::setup()`; `loop()` calls `PanelGroup::loop()`. See
[Your First Panel](first-panel.md) for the minimal shape and [Control Types](../firmware/control-types.md)
for the full catalogue.

!!! note "Most control types aren't implemented yet"
    Today only `LED` and `Switch2Pos` exist. If your panel needs another type, you may be
    implementing it — see [Adding a New Control Type](new-control-type.md).

## Step 4 — Design the PCB

Scaffold the KiCad project with `/new-kicad-project`, capture the schematic, lay out the board.
See [KiCad Workflow](../hardware/kicad-workflow.md) and [PCB Design Rules](../hardware/pcb-design-rules.md).
Order it via [PCB Ordering](pcb-ordering.md).

## Step 5 — Build, flash, bring up

[Assemble](assembly.md) the board, [flash](flashing.md) it, and [bring it up](bring-up.md)
against the bus and DCS.

## Step 6 — Open the PR

Branch `feat/<panel>`, keep it focused, make sure CI passes. Include the `NODE_IDS.md` change.
See [Design Conventions](../contributing/conventions.md).
