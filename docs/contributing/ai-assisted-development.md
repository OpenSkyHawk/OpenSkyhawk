# AI-Assisted Development

OpenSkyhawk is built with [Claude Code](https://www.anthropic.com/claude-code) for firmware,
PCB, and panel-research work. The repo ships a set of **skills** — focused reference packs that
load automatically when you work on a given discipline — so the assistant has the right context
without a giant always-on prompt.

## How the skills work

Skills live in `.claude/skills/<name>/SKILL.md`. Each has a description that tells the model
*when* it applies; Claude Code loads a skill on its own when your task matches, so you usually
don't invoke them by hand. They're plain Markdown — readable by any contributor (and by other
AI tools) as a map of where the deep reference lives.

The skills **point to** the canonical references rather than copying them, so there's a single
source of truth:

- discipline reference: `docs/_source/*.md`
- firmware spec: `Firmware/ScratchPad/FirmwarePlan/` (the *what*) and `TechSpec/` (the *how*)
- project rules + tracking: `CLAUDE.md` (and `AGENTS.md` for other agents)

## The five skills

| Skill | Loads when you're… | Points to |
|-------|--------------------|-----------|
| `panel-mapping` | researching a new panel in the DCS Model Viewer — identifying controls and their types, mapping DCS-BIOS IDs, measuring panel size | GitHub Project (Panel Research & Assignment), `docs/_source/controllers/` |
| `pcb-design` | capturing a schematic or laying out a PCB — parts, packages, connectors, net classes, DRC, wiring an MCP23017/ADS1115, board review | `docs/_source/hardware-standards.md`, `pcb-design-rules.md`, `kicad.md` |
| `firmware` | writing firmware — PanelGroup / PanelBridge / SimGateway, CAN, NODE_ID, DCS-BIOS, HID, libraries and their tests | `FirmwarePlan/`, `TechSpec/` |
| `full-stack` | tracing or debugging across layers, or onboarding — how a control flows from panel to DCS and back | `FirmwarePlan/01-system-overview.md`, the [Architecture](../architecture/index.md) pages |
| `cad` | modeling a panel or bezel, exporting STL/STEP, checking mechanical fit | [CAD Workflow](../hardware/cad-workflow.md) (tooling pending — see [D8](../architecture/design-decisions.md)) |

## The per-panel pipeline

The skills mirror how a panel actually gets built:

```
panel-mapping ──► control inventory + types ──► firmware (input/output classes)
      │                                    └──► pcb-design (GPIO / ADC allocation)
      └──────────► panel dimensions ───────────► cad (model) + pcb-design (board outline)
```

`full-stack` is the glue — reach for it when a question spans more than one of the above, and
it'll hand you off to the right specialist skill.

## Working in the repo

- The always-on context is `CLAUDE.md` — it stays deliberately small: project orientation, the
  skill map above, and the source-of-truth/tracking rules (GitHub Projects for panels, Notion for non-panel tasks, + the `docs-drift` GitHub
  issue). Everything discipline-specific is in a skill.
- Other AI tools should read `AGENTS.md`, which points at the same sources.
- See [Design Conventions](conventions.md) for the rules every change follows, and
  [Adding a Controller](adding-controller.md) for the end-to-end panel walkthrough.
