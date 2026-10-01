# Design Conventions

The conventions that keep OpenSkyhawk consistent across boards, firmware, and contributors.
None of these are negotiable in a PR — they're what makes one person's panel interchangeable
with another's.

## NODE_IDs are permanent

Every CAN board has a unique `NODE_ID` (1–63; 0 is PanelBridge). Set it via
`build_flags = -DNODE_ID=N`, never a `#define` in `main.cpp`. Claim it in `Firmware/NODE_IDS.md`
**before** you start, in the same PR that creates the sketch, and **never reuse one** — retire,
don't recycle. Full rules: [NODE_ID & CAN Addressing](../firmware/node-id.md).

## Naming

- **DCS-BIOS output addresses:** `A_4E_C_<NAME>` for the address, `A_4E_C_<NAME>_AM` for the
  mask. **Never** the old `_A` suffix that DCS-BIOS's own headers use.
- **DCS-BIOS input commands:** the compact `DCSIN_*` constants from `A4EC_CmdIds.h`.
- **HID controls:** the `CTRL_*` constants from `HIDControls.h` (the authoritative source — if
  a doc disagrees with the header, the header wins).

## No magic numbers

Always use the named helper functions for CAN IDs (`canIdHb(n)`, `canIdEvt(n)`, …) — never the
raw `0x100 + n` arithmetic. The deprecated fixed constants (`CAN_ID_HB_1 = 0x100`, etc.) are
off-by-one against the helpers; don't use them.

## Wiring map per sketch

Every `PinRef` bit position and bitmask must be a **named constant**, never an inline literal.
Define a wiring-map block at the top of each sketch — one named `PinRef` per physical
connection, matching the schematic net label. Every pin then appears exactly once and traces
straight back to the schematic.

## Status badges in docs

Pages and panels carry a consistent status (front-matter `status:`):

| Badge | Meaning |
|-------|---------|
| *(none)* | Complete — real hardware exists and is tested |
| `new` | Recently added, in active development |
| `planned` | Designed but not yet built |
| `stub` | Placeholder, contributions welcome |

Don't present Phase 4/5 control types, planned panels, or the stepper driver as available — mark
them honestly. The reverse is just as wrong: don't leave a class that has shipped sitting in a
"not yet implemented" list.

## Ship docs with the feature

If a change is visible from the published docs, update them in the **same PR** — a new control
class, a new CAN frame family, or a change to what an existing frame carries. Three pages go
stale fastest:

| Page | What it must not lag |
|------|----------------------|
| [Firmware Overview](../firmware/index.md) | what is actually built |
| [Control Types](../firmware/control-types.md) | per-class status |
| [CAN Bus](../architecture/can-bus.md) | the frame ID table |

Keep it at feature/overview altitude. The deep contracts live in `Firmware/ScratchPad/TechSpec/`;
a published page points at them rather than copying them.

The weekly docs drift review catches what slips through, but it is a backstop — it runs after the
fact, and a finding can sit for weeks. Four of the five findings in #284 had been wrong since
earlier merges, each one left behind by the PR that shipped the feature.

## CAD files

- **Source files** (`.f3d` or `.FCStd`) are **committed** to the repo under `CAD/`.
- **STL and STEP exports are gitignored** and published via **GitHub Releases** — never commit
  them, and never describe STLs as living in the repo.
- Keep CAD docs **tool-agnostic** until the Fusion-vs-FreeCAD decision is final — no
  tool-specific how-to yet.

## Don't commit build output

`site/` (MkDocs build) and generated `docs/api/` detail pages are gitignored — don't commit
them. See [Docs Workflow](docs-workflow.md).

## Commits and PR titles

Use **[Conventional Commits](https://www.conventionalcommits.org/)** — `type(scope): summary` —
for **both commit messages and PR titles**. The repo squash-merges with the **PR title as the whole
commit on `main`** — that's what release-please reads for the version bump and changelog of both
firmware and hardware. The **PR Title** check fails a pull request whose title isn't a
Conventional Commit — otherwise the change would silently drop out of the release.

The PR **description** isn't copied into the commit, so write it freely — code snippets included.
It stays on the PR, one click from the `(#123)` in the commit title.

- **Types:** `feat` (new capability), `fix`, `chore`, `test`, `docs`, `refactor`, `perf`, `build`, `ci`.
- **Scope** (optional, encouraged) names the area: `feat(panelbridge): …`, `fix(pcb): …`. For
  hardware use the discipline — `pcb` (KiCad: schematic, layout, silkscreen, footprints), later
  `cad` (panels, bezels, knobs).
- **Breaking change:** `type(scope)!: …`, or a `BREAKING CHANGE:` footer.

```text
feat(firmware): add RotaryEncoder REL/DIR relative dispatch
fix(gen_a4ec): commit id_ledger.json on metadata refresh
docs(contributing): document the conventional-commit standard
```

Do **not** add `Co-Authored-By:` trailers or AI-attribution signatures — see
[AI-Assisted Development](ai-assisted-development.md).

## Releases

Firmware and hardware are versioned separately, both through release-please — the model and its
reasons are [design decision D10](../architecture/design-decisions.md#d10-versioning-firmware-and-hardware-release-separately).

- **Firmware** releases itself. On every push to `main`, release-please updates one draft
  release PR (`chore(main): release firmware X.Y.Z`) with the next version, the
  `Firmware/CHANGELOG.md` entry, and every `library.json` bumped together. The PR stays a
  **draft** so it can't be merged by accident. **Marking it ready and merging it is the
  release:** it tags `firmware-vX.Y.Z` and publishes the GitHub Release. Don't edit the release
  PR — release-please rebuilds it from `main` whenever its contents change; reword notes on the
  published GitHub Release instead.
- Only `feat`, `fix`, `perf`, and breaking changes under `Firmware/` move the version and appear
  in the changelog; `docs`, `chore`, `test` and the rest don't, and neither do commits that touch
  only `Firmware/ScratchPad/`.
- **Adding a firmware library?** Add its `library.json` to `extra-files` in
  `release-please-config.json`, so its version moves with the rest.
- **Hardware** works the same way: commits under `PCB/` feed a draft `release hardware X.Y.Z` PR
  (`PCB/CHANGELOG.md`, `release:` in `PCB/manifest.yaml`); merging it tags `hardware-vX.Y.Z`. What
  you add by hand is the **manifest** — a board is listed there only once it has been built and
  verified. Mark a hardware break (a pinout, harness, mounting or minimum-firmware change) with
  `!`: `feat(pcb)!: move J_SR to pin header`. A commit touching both `Firmware/` and `PCB/` counts
  for both releases. Standard and checklist: `docs/_source/hardware-standards.md` (*Releases*).
- **Tags are only for releases.** release-please creates them; never tag by hand.

### Fixing a release entry

If a merged PR is missing from a release PR, or its changelog line is wrong, don't edit the
release PR — release-please rebuilds it. Instead, add this block to the **merged PR's
description**, with the line you want:

```text
BEGIN_COMMIT_OVERRIDE
feat(firmware): DrumDisplay takes a bus or a mux (#318)
END_COMMIT_OVERRIDE
```

then re-run the **Release** workflow (Actions → Release → Run workflow). release-please reads the
override instead of the commit, and the release PR updates. Several lines in the block become
several changelog entries.
