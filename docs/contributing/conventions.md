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
for **both commit messages and PR titles**. The repo squash-merges, so the **PR title becomes the
commit on `main`** that drives the release-version automation; a non-conventional title breaks the
version bump.

- **Types:** `feat` (new capability), `fix`, `chore`, `test`, `docs`, `refactor`, `perf`, `build`, `ci`.
- **Scope** (optional, encouraged) names the area: `feat(panelbridge): …`, `fix(pcb): …`.
- **Breaking change:** `type(scope)!: …`, or a `BREAKING CHANGE:` footer.

```text
feat(firmware): add RotaryEncoder REL/DIR relative dispatch
fix(gen_a4ec): commit id_ledger.json on metadata refresh
docs(contributing): document the conventional-commit standard
```

Do **not** add `Co-Authored-By:` trailers or AI-attribution signatures — see
[AI-Assisted Development](ai-assisted-development.md).
