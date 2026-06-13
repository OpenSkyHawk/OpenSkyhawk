# Adding a Controller

Adding a new panel controller — a new PanelGroup node for a cockpit panel — touches all three
disciplines. This page is the map of how they fit together; each step links to the detailed
reference or guide.

## The shape of it

A panel controller is one STM32 board (the PanelGroup node) plus its breakouts, running a
sketch, mounted in a printed enclosure. Bringing one to life:

1. **Claim a NODE_ID.** Take the next free number in `Firmware/NODE_IDS.md` and commit it in the
   PR that starts the work. This is always step one — see
   [NODE_ID & CAN Addressing](../firmware/node-id.md).
2. **Design the PCB.** Scaffold a KiCad project with `/new-kicad-project`, capture the schematic,
   lay out the board (LEDs front, everything else back), and get ERC/DRC clean. See the
   [KiCad Workflow](../hardware/kicad-workflow.md) and [PCB Design Rules](../hardware/pcb-design-rules.md).
3. **Write the firmware.** Copy a template from `Firmware/Templates/`, set `-DNODE_ID`, build the
   wiring map, and declare your control objects. See [PlatformIO Setup](../firmware/platformio-setup.md)
   and [Control Types](../firmware/control-types.md).
4. **Model the enclosure.** The printed panel, bezel, and any knobs. CAD tooling is still under
   evaluation — see [CAD Workflow](../hardware/cad-workflow.md).

## Choosing how each control routes

For every switch, knob, LED, and gauge, decide the path: **DCS-BIOS** (default — anything with a
DCS-BIOS address) or **HID** (flight-control axes and momentary controls with no DCS-BIOS
address). This is the single most important per-control decision — read
[DCS-BIOS vs HID](../architecture/dcsbios-vs-hid.md) before you start declaring controls.

## Follow the conventions

Named constants and a wiring map, permanent NODE_IDs, `A_4E_C_*` / `_AM` naming, honest status
badges — all in [Design Conventions](conventions.md).

!!! note "Detailed step-by-step guides are coming"
    The full worked walkthroughs — [Your First Panel](../guides/first-panel.md),
    [Adding a New Panel Group](../guides/new-panel-group.md), and
    [Adding a New Control Type](../guides/new-control-type.md) — are in the Build Guides section
    and being written. This page is the orientation; those are the recipes.
