# Contributing to OpenSkyhawk

Thanks for your interest in contributing to the OpenSkyhawk A-4E Skyhawk home cockpit project.

> **Full contributing guide:** See `docs/contributing/` (coming soon — documentation milestone in progress).
> This file covers the essentials to get you started.

## Toolchain Setup

You need:

- **PlatformIO** — firmware builds. Install via VS Code extension or `pip install platformio`.
- **KiCad 10.x** — PCB and schematic work. Download from [kicad.org](https://www.kicad.org/).
- **Fusion 360** — CAD panel models (free for hobbyists).
- **Python 3.12+** — tools in `tools/`.
- **ST-Link v2** — STM32 flashing.

Clone the repo, open any `Firmware/<project>/` folder in VS Code with PlatformIO installed — it will auto-configure.

## Claiming a NODE_ID

Every panel group board needs a unique `NODE_ID` (1–63). NODE_IDs are permanent once a board is flashed.

1. Check `Firmware/NODE_IDS.md` for the current registry and next available ID.
2. Open a PR that adds your board to the registry **before** starting firmware work.
3. Never reuse a NODE_ID, even if a panel is retired.

## Pull Request Process

1. Branch from `main`. Use prefixes: `feat/`, `fix/`, `chore/`, `docs/`, `ci/`.
2. Keep PRs focused — one issue per PR.
3. Firmware: verify `pio run` compiles before pushing.
4. PCB: run KiCad ERC before pushing; DRC clean is required for layout PRs.
5. CI must pass — do not merge with failing checks.

## AI-Assisted Development

This project uses Claude Code (Anthropic) for firmware and PCB work. See `AGENTS.md` for conventions and context that apply when working with AI coding assistants in this repo.

## Code of Conduct

All contributors are expected to follow the [Code of Conduct](CODE_OF_CONDUCT.md).
