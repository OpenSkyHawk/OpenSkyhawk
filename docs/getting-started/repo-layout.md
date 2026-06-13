---
status: new
---

# Repo Layout

!!! warning "This reflects the target structure"
    The repository is being restructured. Some directories shown here are in the process of
    being moved, renamed, or created. This page describes the **target state** — where things
    are headed — so contributors orient against the intended layout, not transient cruft.

```
OpenSkyhawk/
├── .github/
│   └── workflows/          ← CI: docs, firmware, KiCad, A4EC generator
├── CAD/                    ← panel enclosures, bezels, knobs (scaffolded, early stage)
│   ├── Center_Console/
│   ├── Left_Console/
│   ├── Right_Console/
│   └── Shared/             ← reusable knobs, guards, light rings
├── Firmware/
│   ├── Examples/
│   │   └── E2E_DCS_Test/   ← working reference sketch (LED + Switch2Pos + real DCS-BIOS)
│   ├── Libraries/          ← production shared libraries
│   ├── Panels/             ← panel-group sketches (one folder per group)
│   ├── PanelBridge/        ← STM32 PanelBridge sketch
│   ├── SimGateway/         ← RP2040 SimGateway sketch
│   ├── Templates/          ← PlatformIO project templates for new boards
│   ├── Tests/              ← test projects
│   └── NODE_IDS.md         ← pointer to the NODE_ID registry docs
├── PCB/
│   ├── Center_Console/     ← KiCad projects, by console then controller group
│   ├── Left_Console/
│   ├── Right_Console/
│   └── Libraries/          ← shared symbols, footprints, sheet templates
├── docs/                   ← this MkDocs site (docs_dir = ".")
├── tools/
│   └── gen_a4ec/           ← Python A4EC DCS-BIOS header generator (maintainer tooling)
├── CONTRIBUTING.md
├── CODE_OF_CONDUCT.md
├── AGENTS.md
└── README.md
```

## Top-level directories

| Directory | What's in it |
|-----------|--------------|
| `.github/workflows/` | CI pipelines — docs build/deploy, firmware builds, KiCad ERC/DRC, and the A4EC generator. |
| `CAD/` | Panel enclosures, bezels, and shared printed parts. Source files committed; STL/STEP exports generated, not committed. Scaffolded, early stage. |
| `Firmware/` | All firmware — shared libraries, the three tier sketches, panel groups, templates, and tests. |
| `PCB/` | One KiCad project per physical board, organised by console then controller group. Shared parts live in `PCB/Libraries/`. |
| `docs/` | This documentation site. |
| `tools/gen_a4ec/` | The Python generator that turns A-4E-C DCS-BIOS definitions into firmware headers. Maintainer-only — see below. |

!!! note "CAD source vs exports"
    CAD source files (`.f3d` or `.FCStd`) are committed to the repo under `CAD/`. **STL and
    STEP exports are gitignored** and published via **GitHub Releases**. CAD tooling is
    currently being evaluated between Fusion 360 and FreeCAD.

## Key directories for contributors

Three places matter most if you're writing firmware:

- **`Firmware/Libraries/`** — the production shared code: `CANProtocol`, `STM32Board`,
  `HIDControls`, `PanelGroup`, `PanelBridge`, `SimGateway`, and the generated `A4EC` headers.
  This is the **authoritative source** for any constant or API. When docs and a header
  disagree, the header wins.
- **`Firmware/Examples/E2E_DCS_Test/`** — a working, hardware-verified reference sketch that
  exercises an LED output and a Switch2Pos input against real DCS-BIOS IDs. The best starting
  point for understanding a PanelGroup sketch end to end. It's used as the worked example in
  [Your First Panel](../guides/first-panel.md).
- **`Firmware/Templates/`** — PlatformIO project templates for PanelBridge, SimGateway, and
  PanelGroup. Start a new panel group by copying from here.

!!! warning "Don't copy a panel sketch as a template"
    Use `Firmware/Templates/` to start a new board — **not** an existing panel group's
    `platformio.ini`. Some in-tree panel sketches carry stale dependencies.

## A note on the A4EC generator

`tools/gen_a4ec/` parses the A-4E-C DCS-BIOS JSON and emits the `A4EC_*` headers committed
under `Firmware/Libraries/`. The generated headers are committed — **contributors do not run
the generator**. Refreshing it when a new A-4E-C release ships is a maintainer task. See the
maintainer documentation for that workflow.
