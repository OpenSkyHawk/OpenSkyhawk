# CLAUDE.md

OpenSkyhawk is a physical DCS A-4E Skyhawk home cockpit build. It produces 3D-printed panels, custom PCBs, and STM32 firmware to replicate the full A-4E cockpit for use with the DCS A-4E Community Mod. Controllers communicate over CAN bus. Project notes and panel status are tracked in Notion under the "A-4E Home Cockpit" workspace.

@Docs/claude/architecture.md
@Docs/claude/hardware-standards.md
@Docs/claude/kicad.md
@Docs/claude/pcb-design-rules.md

## Licensing

| Layer | License |
|---|---|
| CAD, PCB, Docs | CC BY-NC-SA 4.0 (root `LICENSE`) |
| Firmware | GPL v2 (`Firmware/LICENSE`) |

## MCP Servers Available

- **Autodesk Fusion** — can read, execute, and update Fusion 360 designs directly
- **Notion** — all project notes live under the "A-4E Home Cockpit" page (Notion ID: `301575ac53b180b6a1b7cce9ba40ac79`), including a Panels database tracking status per panel

### Notion Workflow Rules

**Always search before creating.** Before calling `notion-create-pages`, search the target database for an existing page with the same or similar name. Update it instead of creating a duplicate.

**Write notes to page body content, not to the Description property.** The `Description` property field is a one-liner used for database-level filtering. Substantive notes (repo paths, scaffolding status, wiring details) go in the page body via `update_content`.

### Panels Database

- **Collection URL:** `collection://301575ac-53b1-80c1-be2d-000b57d99f55`
- **Database page:** `https://www.notion.so/301575ac53b180b2ad55d2b6394d0b25`
- **Key properties:** `Task name` (title), `Status`, `Console Position`, `Controller`, `Panel Type`, `Priority`
- **Status options:** Not started → Research → CAD → Prototyping → Building → Done
- **When a KiCad project is scaffolded:** find the existing panel page, set Status to `Not started` (unchanged if already set), and add the repo path + scaffolding note to the **page body** under the Task description section.

## Git

- Remote: `git@github.com:mottihoresh/OpenSkyhawk.git`
- Do not add `Co-Authored-By:` trailers to commits
- STLs, Gerbers, and other generated outputs are gitignored — commit sources only
