# CLAUDE.md

OpenSkyhawk is a physical DCS A-4E Skyhawk home cockpit build. It produces 3D-printed panels, custom PCBs, and STM32 firmware to replicate the full A-4E cockpit for use with the DCS A-4E Community Mod. Controllers communicate over CAN bus. Project notes and panel status are tracked in Notion under the "A-4E Home Cockpit" workspace.

@Docs/claude/architecture.md
@Docs/claude/hardware-standards.md
@Docs/claude/kicad.md

## Licensing

| Layer | License |
|---|---|
| CAD, PCB, Docs | CC BY-NC-SA 4.0 (root `LICENSE`) |
| Firmware | GPL v2 (`Firmware/LICENSE`) |

## MCP Servers Available

- **Autodesk Fusion** — can read, execute, and update Fusion 360 designs directly
- **Notion** — all project notes live under the "A-4E Home Cockpit" page (Notion ID: `301575ac53b180b6a1b7cce9ba40ac79`), including a Panels database (with `Controller` field) tracking status per panel

## Git

- Remote: `git@github.com:mottihoresh/OpenSkyhawk.git`
- Do not add `Co-Authored-By:` trailers to commits
- STLs, Gerbers, and other generated outputs are gitignored — commit sources only
