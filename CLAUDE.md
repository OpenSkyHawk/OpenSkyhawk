# CLAUDE.md

OpenSkyhawk is a physical DCS A-4E Skyhawk home cockpit build. It produces 3D-printed panels, custom PCBs, and STM32 firmware to replicate the full A-4E cockpit for use with the DCS A-4E Community Mod. Controllers communicate over CAN bus. Project notes and panel status are tracked in Notion under the "A-4E Home Cockpit" workspace.

@docs/claude/architecture.md
@docs/claude/hardware-standards.md
@docs/claude/kicad.md
@docs/claude/pcb-design-rules.md
@Firmware/ScratchPad/FirmwarePlan/README.md
@Firmware/ScratchPad/TechSpec/README.md

## Firmware API

Generated from Doxygen docblocks by `tools/doxy2md.py`. Re-run `doxygen Doxyfile && python3 tools/doxy2md.py --xml .doxygen/xml --out docs/api` after changing library headers.

@docs/api/CANProtocol_h.md
@docs/api/STM32Board.md
@docs/api/PanelBridge.md
@docs/api/PanelGroup.md
@docs/api/SimGateway.md
@docs/api/HIDControls_h.md

## Firmware

All firmware architecture decisions, data flows, and contract boundaries are defined in
`Firmware/ScratchPad/FirmwarePlan/` — **this is the authoritative source of truth** and
supersedes any firmware content in `docs/claude/`. Implementation-level technical
specifications (public API, class structure, method signatures) live in
`Firmware/ScratchPad/TechSpec/`.

Do not rely on the Firmware Architecture section of `docs/claude/architecture.md` for
firmware decisions — read the FirmwarePlan instead.

## Licensing

| Layer | License |
|---|---|
| CAD, PCB, Docs | CC BY-NC-SA 4.0 (root `LICENSE`) |
| Firmware | GPL v2 (`Firmware/LICENSE`) |

## MCP Servers Available

- **Autodesk Fusion** — can read, execute, and update Fusion 360 designs directly
- **Notion** — all project notes live under the "A-4E Home Cockpit" page (Notion ID: `301575ac53b180b6a1b7cce9ba40ac79`), with two databases: **Panels** (one page per physical panel) and **Tasks** (non-panel work items)

### Notion Workflow Rules

**Always search before creating.** Before calling `notion-create-pages`, search the target database for an existing page with the same or similar name. Update it instead of creating a duplicate.

**Write notes to page body content, not to the Description property.** The `Description` property field is a one-liner used for database-level filtering. Substantive notes (repo paths, scaffolding status, wiring details) go in the page body via `update_content`.

**Finding a page:** use `notion-search` with the panel or task name, then `notion-fetch` on the result URL to read its current content before editing.

### Panels Database

Tracks every physical panel in the cockpit — one page per panel. Purpose: specification and implementation record (components, PCB repo path, wiring, DCS-BIOS IDs). Not for general task tracking.

- **Collection URL:** `collection://301575ac-53b1-80c1-be2d-000b57d99f55`
- **Database page:** `https://www.notion.so/301575ac53b180b2ad55d2b6394d0b25`
- **Key properties:** `Task name` (title), `Status`, `Console Position`, `Controller`, `Panel Type`, `Priority`
- **Status pipeline:** Not started → Research → Schematics → CAD → PCB Layout → Ordering → Assembly → Testing → Done
  - **Schematics** — KiCad schematic complete, ERC clean
  - **CAD** — Fusion 360 panel model in progress or complete
  - **PCB Layout** — KiCad PCB layout in progress or complete
  - **Ordering** — PCB sent to JLCPCB, components sourced
  - **Assembly** — board soldered and installed in panel
  - **Testing** — functional verification in DCS
- **When a KiCad project is scaffolded:** find the existing panel page, set Status to `Not started` (unchanged if already set), and add the repo path + scaffolding note to the **page body** under the Task description section.
- **When schematics are complete:** update page body with sheet structure, key ICs, harness connectors, and DCS-BIOS IDs. Advance Status to `Schematics`.

### Tasks Database

Tracks non-panel work items — firmware milestones, architecture decisions, library updates, CAN integration, and other build tasks. Use this instead of the Panels database for anything that isn't tied to a specific panel.

- **Collection URL:** `collection://188fa7e5-b170-498b-9c3b-ed7fc0c71138`
- **Database page:** `https://www.notion.so/6d9b009c17d447599431d373484f510d`
- **Key properties:** `Task` (title), `Status`, `Category`, `Priority`
- **Status options:** Backlog → In Progress → Done
- **Category options:** Firmware, Hardware, Architecture, Library, PCB, Documentation, Other
- **Notes go in the page body**, not in any property field.
- **When starting a new non-panel work item:** search Tasks first, then create a new page if none exists. Set Category and Priority at creation time.

## Git

- Remote: `git@github.com:OpenSkyHawk/OpenSkyhawk.git`
- Do not add `Co-Authored-By:` trailers to commits
- STLs, Gerbers, and other generated outputs are gitignored — commit sources only
