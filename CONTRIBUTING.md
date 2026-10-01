# Contributing to OpenSkyhawk

Thanks for your interest in contributing to the OpenSkyhawk A-4E Skyhawk home cockpit project.

> **Full contributing guide:** See `docs/contributing/` (coming soon — documentation milestone in progress).
> This file covers the essentials to get you started.

## Toolchain Setup

You need:

- **PlatformIO** — firmware builds. Install via VS Code extension or `pip install platformio`.
- **KiCad 10.x** — PCB and schematic work. Download from [kicad.org](https://www.kicad.org/).
- **CAD tool** — for panel models. Tooling is under evaluation (Fusion 360 vs FreeCAD); FreeCAD is the preferred direction.
- **Python 3.12+** — tools in `tools/`.
- **ST-Link v2** — STM32 flashing.

Clone the repo, open any `Firmware/<project>/` folder in VS Code with PlatformIO installed — it will auto-configure.

## Claiming a NODE_ID

Every panel group board needs a unique `NODE_ID` (1–63). NODE_IDs are permanent once a board is flashed.

1. Check `Firmware/NODE_IDS.md` for the current registry and next available ID.
2. Open a PR that adds your board to the registry **before** starting firmware work.
3. Never reuse a NODE_ID, even if a panel is retired.

## Pull Request Process

1. Branch from `main`. Use prefixes: `feat/`, `fix/`, `chore/`, `docs/`, `ci/`, `build/`.
2. Keep PRs focused — one issue per PR.
3. Title the PR as a Conventional Commit — `type(scope): summary` (see below). A required check enforces it.
4. Firmware: verify `pio run` compiles before pushing.
5. PCB: run KiCad ERC before pushing; DRC clean is required for layout PRs.
6. CI must pass — do not merge with failing checks.

## How Releases Work

- **Your PR title matters.** Write it as `type(scope): summary` — e.g. `feat(firmware): add a dimmer output` or `fix(pcb): swap the TVS diode`. A required check fails the PR if it isn't; the title becomes the line in the changelog.
- **Firmware releases itself.** Each merge to `main` keeps a draft *"release firmware X.Y.Z"* pull request up to date with the next version and its changelog. When a maintainer marks it ready and merges it, the release is tagged and published — nothing for you to do.
- **Hardware releases the same way.** Commits under `PCB/` keep a draft *"release hardware X.Y.Z"* pull request up to date. A board joins a release once it has been built and verified — it's then listed in `PCB/manifest.yaml` — and each board folder's `README.md` says whether it is Released, In progress, or Deprecated.

Want the details? See *Releases* in [Design Conventions](docs/contributing/conventions.md#releases) and design decision D10 in [Design Decisions](docs/architecture/design-decisions.md).

## AI-Assisted Development

This project uses Claude Code (Anthropic) for firmware, PCB, and panel-research work. Discipline reference is packaged as **skills** under `.claude/skills/` (`panel-mapping`, `pcb-design`, `firmware`, `full-stack`, `cad`) that load automatically by task; `CLAUDE.md` holds the always-on project rules and the skill map, and `AGENTS.md` points other agents at the same sources. See the [AI-Assisted Development](https://openskyhawk.github.io/OpenSkyhawk/contributing/ai-assisted-development/) page for the full rundown.

## Code of Conduct

All contributors are expected to follow the [Code of Conduct](CODE_OF_CONDUCT.md).
