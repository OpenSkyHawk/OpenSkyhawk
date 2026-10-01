# How to Contribute

OpenSkyhawk is built in the open and there's plenty to do. Whether you write firmware, lay out
PCBs, model panels, or just want to improve the docs, this section explains how the project
works and the conventions to follow.

## Three disciplines

A cockpit panel is three things at once, and the project is organised the same way:

- **Firmware** — the control logic on STM32/RP2040. Start with the
  [Firmware Reference](../firmware/index.md).
- **PCB hardware** — KiCad schematics and boards. Start with
  [Hardware Reference](../hardware/standards.md) and the
  [KiCad Workflow](../hardware/kicad-workflow.md).
- **CAD / mechanical** — the printed/machined panel enclosures, bezels, and knobs. See
  [CAD Workflow](../hardware/cad-workflow.md) (tooling still being decided).

Most panels need all three. The [Adding a Controller](adding-controller.md) page walks through
how they fit together.

## Ways to help

- **Build a planned panel.** The left and right consoles are open ground — pick a panel group
  and own it. The [Armament Group](../panels/center-console/armament/index.md) is the worked
  example to follow.
- **Implement a control type.** Most input/output types are specified but not yet written — see
  [Control Types](../firmware/control-types.md) for what's open.
- **Improve the docs.** Every stub page is an invitation. See [Docs Workflow](docs-workflow.md).

## Before you start

The canonical setup and PR essentials live in **`CONTRIBUTING.md`** at the repo root — toolchain
install, claiming a NODE_ID, the PR process, and the
[Code of Conduct](https://github.com/OpenSkyhawk/OpenSkyhawk/blob/main/CODE_OF_CONDUCT.md). The
short version:

- Branch from `main` with a `feat/` `fix/` `chore/` `docs/` `ci/` `build/` prefix; one focused change per PR.
- Title the PR as a Conventional Commit, `type(scope): summary` — a required check enforces it.
- Firmware: `pio run` must compile. PCB: ERC clean (DRC clean for layout PRs).
- **CI must pass — never merge with failing checks.**
- Claim your `NODE_ID` in `Firmware/NODE_IDS.md` *before* starting firmware work.

## How releases work

- **Your PR title matters.** Write it as `type(scope): summary` — e.g.
  `feat(firmware): add a dimmer output` or `fix(pcb): swap the TVS diode`. A required check fails
  the PR if it isn't; the title becomes the line in the changelog.
- **Firmware releases itself.** Each merge to `main` keeps a draft *"release firmware X.Y.Z"* pull
  request up to date with the next version and its changelog. When a maintainer marks it ready
  and merges it, the release is tagged and published — nothing for you to do.
- **Boards and the cockpit are released by hand.** A board gets a version tag when it's sent to be
  made; the whole cockpit gets a release once its boards have been tested together.

Want the details? See [Releases](conventions.md#releases) and
[design decision D10](../architecture/design-decisions.md#d10-versioning-one-repo-three-release-tracks).

## In this section

- **[Design Conventions](conventions.md)** — naming, NODE_IDs, wiring maps, CAD files, the rules that keep things consistent
- **[Docs Workflow](docs-workflow.md)** — how to build, preview, and ship a docs change
- **[Adding a Controller](adding-controller.md)** — the cross-discipline path for a new panel
