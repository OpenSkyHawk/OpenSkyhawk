# CLAUDE.md

OpenSkyhawk is a physical DCS A-4E Skyhawk home cockpit build. It produces 3D-printed panels, custom PCBs, and STM32 firmware to replicate the full A-4E cockpit for use with the DCS A-4E Community Mod. Controllers communicate over CAN bus. Panel/controller build tracking lives in **GitHub Projects** (org `OpenSkyHawk`); non-panel tasks and project notes are in **Notion** under the "A-4E Home Cockpit" workspace.

## Repository structure

Organized by discipline, then by console position (Left / Center / Right):

- `CAD/` — source CAD models (`.f3d` or `.FCStd`; tooling under evaluation, Fusion 360 vs FreeCAD — design decision D8). STL/STEP exports are gitignored; generate from source.
- `PCB/` — KiCad projects, one per physical PCB under `PCB/<Console>/<Group>/<Board>/`. `PCB/Libraries/` holds shared symbols, footprints, and sheet templates.
- `Firmware/` — PlatformIO projects. `Firmware/Libraries/` (shared code, STM32 + RP2040 split), `Firmware/Panels/` (production sketches), `Firmware/Tests/` (per-library test projects). The authoritative firmware spec lives in `Firmware/ScratchPad/FirmwarePlan/` (+ `TechSpec/`).
- `docs/` — MkDocs Material site (GitHub Pages). `docs/_source/` holds the canonical discipline reference material (not in the published nav); `docs/api/` is mkdoxy-generated.

## Skills — discipline reference loads on demand

Deep, discipline-specific reference lives in **model-invoked skills** under `.claude/skills/`,
which load automatically when you work on that aspect (don't paste their content here):

| Working on… | Skill |
|---|---|
| KiCad schematics / PCB layout, part & package selection, connectors, DRC, wiring & board review | `pcb-design` |
| Firmware — PanelGroup / PanelBridge / SimGateway, CAN, NODE_ID, DCS-BIOS, HID, libraries & tests | `firmware` |
| How the whole stack fits together / cross-layer questions / onboarding / debugging across layers | `full-stack` |
| CAD panel & bezel modeling, STL/STEP export, mechanical fit | `cad` |
| Researching a new panel — DCS Model Viewer, control inventory & types, panel sizing | `panel-mapping` |
| Driving a panel/controller through the full build pipeline (research → schematic → CAD → PCB → firmware → order → test), grouping panels into controllers, or resuming a partly-built one | `panel-pipeline` |

Contributor-facing version: [AI-Assisted Development](docs/contributing/ai-assisted-development.md).

**Firmware source of truth:** `Firmware/ScratchPad/FirmwarePlan/` is authoritative for all firmware behaviour and **supersedes** any firmware content in `docs/_source/`. The `firmware` skill carries the routing and conventions.

## Source of truth & tracking

Two systems hold living project state: **GitHub Projects** for panel/controller build tracking, and **Notion** for non-panel tasks + project notes. Keep them in sync as work lands.

### GitHub Projects — panel/controller tracking (org `OpenSkyHawk`)

**Panels and controllers are tracked in GitHub Projects v2, not Notion.** **Git holds the reference data** (`docs/_source/a4ec-control-inventory.csv` — the curated master control inventory; usage in `a4ec-control-inventory.md`); the panel→console/controller assignment + build state live in the **Projects** (the tracker). The **`panel-pipeline` skill owns the structure** (fields, the controller→panel sub-issue hierarchy, the graduation procedure). Fetch live field/option node IDs with `gh project field-list <n> --owner OpenSkyHawk --format json`.

- **#1 Panel Research & Assignment** — every panel as a **draft** through research/assignment (fields: Console, #Controls, Breakdown, Panel Type, Priority, + independent flag columns `Controls?` / `Screenshot?` / `Analysis?`).
- **#2 Controller Build** — each controller as an **issue** through the build pipeline (B1–B9), with its panels as **sub-issues**; per-stage work tracked as task-list checklists.
- Build-stage state = the Project `Status` field cross-checked against **repo signals** (KiCad ERC/DRC, exported gerbers, firmware compiles). ModelViewer reference screenshots live as **committed repo files** — issue/Notion image attachments aren't reliably fetchable.

### Notion — "A-4E Home Cockpit" workspace (Notion ID: `301575ac53b180b6a1b7cce9ba40ac79`)

Notion holds the **Tasks** database (non-panel work items) + general project notes. *(The legacy Notion Panels database is superseded by the GitHub Projects above — research-reference only; do not treat it as the tracker.)*

**Always search before creating.** Before calling `notion-create-pages`, search the Tasks database for an existing page with the same or similar name. Update it instead of creating a duplicate.

**Write notes to page body content, not to the Description property.** The `Description` property field is a one-liner used for database-level filtering. Substantive notes go in the page body via `update_content`.

**Finding a page:** use `notion-search` with the task name, then `notion-fetch` on the result URL to read its current content before editing.

#### Tasks Database

Tracks non-panel work items — firmware milestones, architecture decisions, library updates, CAN integration, and other build tasks. Use this instead of the Panels database for anything that isn't tied to a specific panel.

- **Collection URL:** `collection://188fa7e5-b170-498b-9c3b-ed7fc0c71138`
- **Database page:** `https://www.notion.so/6d9b009c17d447599431d373484f510d`
- **Key properties:** `Task` (title), `Status`, `Category`, `Priority`
- **Status options:** Backlog → In Progress → Done
- **Category options:** Firmware, Hardware, Architecture, Library, PCB, Documentation, Other
- **Notes go in the page body**, not in any property field.
- **When starting a new non-panel work item:** search Tasks first, then create a new page if none exists. Set Category and Priority at creation time.

### GitHub issues — docs drift

The weekly docs-drift review (`tools/docs-drift/REVIEW.md`) opens/updates a single issue labelled `docs-drift` when the published docs fall behind a source of truth. Triage from there; don't open duplicates.

## Licensing

| Layer | License |
|---|---|
| CAD, PCB, Docs | CC BY-NC-SA 4.0 (root `LICENSE`) |
| Firmware | GPL v2 (`Firmware/LICENSE`) |

## MCP servers available

- **Autodesk Fusion** — read, execute, and update Fusion 360 designs directly (CAD tooling still under evaluation — see D8).
- **Notion** — the project workspace above; edit rules in the *Source of truth & tracking* section.

## Git

- Remote: `git@github.com:OpenSkyHawk/OpenSkyhawk.git`
- Do not add `Co-Authored-By:` trailers to commits
- STLs, Gerbers, and other generated outputs are gitignored — commit sources only
