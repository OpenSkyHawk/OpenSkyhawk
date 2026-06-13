# AGENTS.md

Guidance for AI coding agents (Codex and others) working in this repository.

OpenSkyhawk is a physical DCS A-4E Skyhawk home cockpit build — 3D-printed panels, custom PCBs,
and STM32/RP2040 firmware that talks to the DCS A-4E Community Mod. Controllers communicate over
CAN bus.

## Where the context lives

`CLAUDE.md` is the authoritative, always-on project guide. Discipline-specific reference is
packaged as **skills** under `.claude/skills/` — read the one matching your task:

| Task | Skill file |
|------|-----------|
| New-panel research (DCS Model Viewer, control inventory & types, sizing) | `.claude/skills/panel-mapping/SKILL.md` |
| KiCad schematic / PCB layout, parts, DRC, wiring | `.claude/skills/pcb-design/SKILL.md` |
| Firmware (PanelGroup / PanelBridge / SimGateway, CAN, NODE_ID, DCS-BIOS) | `.claude/skills/firmware/SKILL.md` |
| Cross-layer / how the whole stack fits | `.claude/skills/full-stack/SKILL.md` |
| CAD / mechanical | `.claude/skills/cad/SKILL.md` |

Canonical source material (skills point here — don't duplicate it):

- `docs/_source/*.md` — architecture, hardware standards, PCB design rules, KiCad notes
- `Firmware/ScratchPad/FirmwarePlan/` — authoritative firmware spec (supersedes `docs/_source/`
  for firmware); `Firmware/ScratchPad/TechSpec/` — per-class implementation specs
- Notion "A-4E Home Cockpit" workspace — live panel/task tracking (rules in `CLAUDE.md`)

## Always-true rules

- **Licensing:** CAD/PCB/Docs = CC BY-NC-SA 4.0 (root `LICENSE`); Firmware = GPL v2
  (`Firmware/LICENSE`).
- **Git:** remote `git@github.com:OpenSkyHawk/OpenSkyhawk.git`; do not add `Co-Authored-By:`
  trailers; commit sources only (STLs, gerbers, and other generated outputs are gitignored).

Keep this file a thin pointer — when project rules change, update `CLAUDE.md` and the relevant
skill, not a second copy here.
