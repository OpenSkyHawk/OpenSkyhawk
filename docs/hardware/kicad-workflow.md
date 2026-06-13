# KiCad Workflow

How OpenSkyhawk KiCad projects are organised, share a common parts library, and get fabbed.
Every physical board is its own KiCad project; shared parts and rules live in `PCB/Libraries/`.

## Shared library — no global setup needed

All projects point at `PCB/Libraries/` as the single source of truth for custom parts. The
library tables use the built-in `${KIPRJMOD}` variable, so they resolve automatically — clone
the repo, open a project, and the library is just there. **No global KiCad configuration.**

The tables resolve three levels up from the board folder, matching the fixed structure:

```
PCB/
├── Libraries/          ← shared library root
└── <Console>/
    └── <Group>/
        └── <Board>/    ← the KiCad project, 3 levels below Libraries
            ├── sym-lib-table
            └── fp-lib-table
```

## Scaffolding a new board — use the skill

**Always create a new board with the `/new-kicad-project` skill — never hand-roll the project
files.** It copies the library tables, pins the OpenSkyhawk libs, pre-loads the
[design rules](pcb-design-rules.md), creates the root schematic, and registers the board in
Notion.

```
/new-kicad-project <Console> <Group> <BoardName> [mcu|breakout]
```

## The shared library

`PCB/Libraries/OpenSkyhawk.kicad_sym` + `OpenSkyhawk.pretty/` hold the custom symbols and
footprints (the 5050 LED, IRLML2502, the X27 steppers). Everything else is a KiCad built-in. The
full parts list with symbol references is on the [Component Library](components.md) page.

To add a new custom part: add the symbol in `OpenSkyhawk.kicad_sym`, create the matching
`.kicad_mod` in `OpenSkyhawk.pretty/`, and commit both — no per-project duplication.

## Reusable sheet templates

`PCB/Libraries/sheets/` is for shared hierarchical sheets (power rail, MCP23017 instance, LED
zone switch, ADC filter, CAN transceiver) — **Place → Add Sheet** and browse to the template.
(README present; the `.kicad_sch` templates aren't populated yet.)

## The KiCad CLI

KiCad 10.0.1 ships a CLI used for validation and fabrication outputs:

```bash
KICAD=/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli

# Validation
$KICAD sch erc --output erc.json <board.kicad_sch>
$KICAD pcb drc --output drc.json <board.kicad_pcb>

# Fabrication
$KICAD pcb export gerbers --output ./gerbers/ <board.kicad_pcb>
$KICAD pcb export drill   --output ./gerbers/ <board.kicad_pcb>
$KICAD sch export bom     --output bom.csv     <board.kicad_sch>

# Fusion/FreeCAD fit-check + panel cutout
$KICAD pcb export step --output board.step  <board.kicad_pcb>
$KICAD pcb export dxf  --output cutout.dxf  <board.kicad_pcb>
```

ERC/DRC also run automatically in CI on PRs that touch PCB files.

## CAD fit-check

The `pcb export step` output drops straight into the mechanical CAD for a board-in-enclosure fit
check. The CAD side of that workflow is on the [CAD Workflow](cad-workflow.md) page (tooling
still under evaluation).
