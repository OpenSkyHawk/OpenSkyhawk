---
name: panel-pipeline
description: Orchestrate building an OpenSkyhawk cockpit controller end-to-end — research → schematic → CAD → backlight → PCB → firmware → order → assembly → test. Use when starting a new panel or controller, advancing one to its next stage, resuming a partially-built one (detect where it stands and continue), or partitioning the cockpit's panels into controllers. A thin orchestrator: it sequences stages, enforces gates, keeps Notion + repo tracking in sync, owns the cross-cutting decisions (controller grouping, NODE_ID, prerequisites), and delegates the discipline work to panel-mapping, pcb-design, cad, firmware, and verify. Multi-session and resumable.
---

# Panel Pipeline — controller build orchestrator

Drives a physical panel/controller from *Not started → Done*. **Thin orchestrator**: it owns
sequencing, gates, cross-cutting decisions (controller grouping, NODE_ID, prerequisites), and
state tracking — it does **not** duplicate discipline depth. Each stage delegates to the owning
skill.

The work spans **multiple sessions** (PCB fab waits, assembly, sim testing). On every invocation:
**first detect where the panel/controller stands, then drive the next stage to its gate and stop**
where the owner is human.

## Detect stage & resume

Read the panel's **Notion Status** (the tracker) and cross-check **repo signals**:

| Stage reached | Repo signal |
|---|---|
| Grouped | NODE_ID claimed in `Firmware/NODE_IDS.md` |
| Researched | `docs/_source/controllers/<Panel>.md` exists + Notion inventory complete |
| Schematic | KiCad project exists + ERC clean |
| CAD | `.FCStd`/`.f3d` source exists |
| Backlight | LED-array count set in schematic + re-ERC clean |
| PCB | routed + DRC clean + gerbers exported |
| Firmware | `Firmware/Panels/<Controller>/` compiles + tests pass |
| Ordering / Assembly / Testing | Notion Status + order ref / boards in hand |

If Notion and the repo disagree, trust the repo signal and correct Notion. *(Conflict-resolution
is a thin detail — refine in use.)*

## Scope: controller, not panel

A **controller** = one STM32 PanelGroup MCU = one CAN **NODE_ID**, hosting a **host panel + I²C
breakouts within ~12–18"**. Notion keeps **one page per physical panel**; the build pipeline runs
**per controller**. The grouping rule + budgets are summarized in A2 below.

## Readiness — pick what's buildable now

A panel/controller is **Ready** when every control type it uses already has an implemented
OpenSkyhawk building block (firmware class **and** KiCad symbol / Design Block); **Blocked** when one
or more are missing.

From the A1 inventory + the implemented-block list, classify the portfolio:

- **Ready** → can run B1–B9 end-to-end with today's toolkit. **Prefer these — build them now.**
- **Blocked** → list the missing blocks (each → a per-board prerequisite ticket, out of scope to
  build here); defer the controller until its blocks land.

This is the main **scheduling lever**: maximize throughput by building everything the current toolkit
supports, while the missing control types are prototyped separately (in parallel). Invoke with
*"what can I build now?"* to get the **Ready** set and pick the next controller.

---

## Phase A — project pre-step (ONCE, before any per-panel mapping)

### A1 — Bulk inventory · *owner: AI*

Parse the **full A4EC mod Lua** (`clickabledata.lua` / `command_defs.lua` / `A-4E-C.lua` — **all**
controls including those not exposed in DCS-BIOS, with native model-viewer IDs and 3D positions)
**plus** the derived **DCS-BIOS JSON**. Per control, record `{name, native ID, type, routing,
positions}`:

- **type** — auto-estimated from the DCS-BIOS type field (selector / toggle_switch / momentary /
  variable_step / analog → firmware class → GPIO/ADC count); from the Lua for HID controls.
- **routing** — control in the JSON → **DCS-BIOS** (record address). Absent from the JSON →
  **HID-routed** (still consumes GPIO/ADC, just a different firmware path). HID controls **must be
  identified** or the I/O budget — and the grouping — is wrong. (Routing is by `controlId` range:
  `DCSIN_*` ≥ 0x8000 → DCS-BIOS via PanelBridge; `CTRL_*` < 0x8000 → HID via SimGateway.)
- **positions** — from the mod files (not manual Model Viewer) → feed A2 distances.

Output: per-panel I/O + actuator budget, recorded in the Notion panel page body. Also run a **bulk
readiness check** — map each control's estimated type → required OpenSkyhawk block → does it exist?
→ mark each panel **Ready** (all blocks exist) or **Blocked** (list missing blocks). Provisional
(estimated types); confirmed at B1 once types are sim-verified. Feeds the Readiness scheduling above.

### A2 — Grouping · *owner: AI proposes → human confirms*

Partition all panels into controllers with the **3 hard gates**, then claim NODE_IDs:

1. **REACH** (hard) — every breakout ≤ **12–18"** I²C harness (mod-file positions × 1.10 → mm).
   Beyond → its own CAN node.
2. **FIT** (hard, expandable) — ≤ 2 I²C buses; ≤ 8 MCP23017/bus (224 inputs); ≤ 4 ADS1115/bus
   (32 analog ch).
3. **PERIPHERALS** — steppers / servos / Hall. Gauges route through an I²C expander at low step
   rates; direct-MCU-GPIO stepping is ~4/MCU. (Full budgets: the controller-grouping design note.)

Bias to **split** — up to 63 CAN nodes are essentially free. Grouping is **provisional here**
(mod-file distances) and **confirmed at B3 CAD** against the real harness path (re-split if a
breakout exceeds 18"). Claim each NODE_ID in `Firmware/NODE_IDS.md`.

---

## Phase B — per-controller build (repeats per controller)

### B1 — Research (deep) · `panel-mapping` · *AI drafts → human photos + in-sim type confirm*

- Confirm the estimated control types **in the real sim**; capture reference photos (human).
- **Dimensions** (× 1.10) + switch **center-to-center spacing** → CAD.
- **Part selection** per control; detailed **control → MCP23017-pin / ADS1115-channel** map.
- **Gap analysis** — for each control type, check the OpenSkyhawk building block exists: **both**
  the **firmware class** (e.g. `OpenSkyhawk::Switch3Pos`) **and** the **KiCad symbol / Design
  Block**. For each missing piece, **create a per-board prerequisite ticket** — a **blocker on this
  specific controller**. A missing **class** blocks **B6**; a missing **symbol** blocks **B2**.
  **Implementing a new control type is OUT OF SCOPE of this pipeline** — it is its own firmware /
  KiCad effort (TechSpec + prototype, e.g. how `Switch2Pos` was built). The pipeline only flags it,
  tickets it, and blocks the board until it lands.
  Implemented today: `Switch2Pos`, `LED`, `PinRef`. Planned/missing: `Switch3Pos`, `ActionButton`,
  `SwitchMultiPos`/`RotarySwitch`, `AnalogInput`, `RotaryEncoder`, `ServoOutput`, `SwitecX25Output`,
  `AccelStepperOutput`, `AngleSensor`, `SwitchWithCover2Pos`.
  This **confirms the A1 provisional Ready/Blocked** classification now that types are sim-verified.
- Out: Controls Inventory + I/O Summary + Dimensions + prerequisites → Notion body +
  `docs/_source/controllers/<Panel>.md`.

### B2 — Schematic · `pcb-design` · *AI analysis → human draws → AI reviews*

- **AI:** scaffold (`/new-kicad-project`) + analyze needs beyond the base PanelGroup (steppers /
  DRV8833, gauges, servos / PCA9685, Hall, extra MCP23017 / ADS1115, backlight). Component list +
  flags.
- **Human draws** the schematic; **AI reviews** (ERC + against hardware-standards + the I/O map).
- **All backlight CONTROL circuitry is defined here** — MOSFET zones, PWM signal nets, LED-string /
  harness connectors. The **only** thing deferred to B4 is the **number of LED arrays**.
- Gate: **ERC clean**.

### B3 — CAD · `cad` · *human-led, AI assists*

- Model the **panel sandwich**: 1/16" face plate (legends) + 1/8" diffuser + 1/8" backplate (LED
  passthrough holes) + the PCB behind it (LEDs on the PCB front face shine forward through the
  backplate holes → diffuser → face).
- Fit-check vs the PCB (connector clearance, stepper shaft-through-PCB, LED positions). Out:
  **lit-area map → B4**.
- **Confirms the A2 grouping** against real harness distance (re-split if a breakout > 18").
- Gate: model + fit-check.

### B4 — Backlight · *AI calc → human confirms zones*

- Size the **number of LED arrays/strings** from the CAD lit-areas (5 × 5050 in series @ 12 V,
  120 Ω/string, IRLML2502/zone — applying the standard, not re-deriving the EE). All control
  circuitry was already defined in B2.
- **Update the B2 schematic** with the array count → **re-ERC**.

### B5 — PCB Layout · `pcb-design` · *human lays out → AI reviews*

- In: the post-B4 schematic + B3 CAD (board outline, LED / connector positions, mounts) +
  `pcb-design-rules`.
- Place (LEDs on the front face, everything else on the back) + route (JLCPCB 2-layer, DRC) →
  **gerbers + BOM** (JLC part numbers).
- **B3 ↔ B5 iterate** — the board outline fits the panel; the PCB-front LED positions must align
  with the backplate passthrough holes.
- Gate: DRC clean + gerbers.

### B6 — Firmware · `firmware` · *AI drafts → human review* · **parallelizable\***

- **Gated by B1, not B5** — needs the I/O map, *not* the physical board. Runs in parallel with
  B2–B5; only needs the board at B9.
- **\*Conditional:** parallelizable **only if no building blocks are missing**. If B1's gap analysis
  ticketed a missing firmware class, B6 is **blocked** on that prerequisite (building it is out of
  scope — a separate ticket) until it lands.
- In: B1 inventory (control → type → pin, DCS-BIOS / HID routing) + NODE_ID + cleared prerequisites.
- PanelGroup sketch (`registerExpander` / `registerADC`, an input/output object per control) +
  A4EC input-map entries + tests.
- Gate: compiles + tests pass.

### B7 Order / B8 Assembly / B9 Test · *human*

- **B7** — gerbers + BOM → JLCPCB order, source parts. Human (financial). **Async** (fab/ship wait).
- **B8** — solder (T962 reflow) + install + harness.
- **B9** — flash + verify each control in DCS (inputs register, gauges/backlight move, CAN reaches
  CONNECTED). AI supplies the checklist (`verify`). → **Done**.

---

## Cross-stage couplings (non-linear)

- **B4 → B2** — set the LED-array count → re-ERC.
- **B3 ↔ B5** — mechanical/electrical co-design (board outline + LED-hole alignment).
- **B1 prerequisites → B2 + B6** — a missing class/symbol gets a per-board blocker ticket;
  implementing it is out of scope (its own firmware/KiCad effort). Missing class → blocks B6;
  missing symbol → blocks B2.
- **B6 ∥ B2–B5** — firmware is parallelizable once B1 is done, **unless** a prerequisite ticket
  blocks it.

## Conventions

- **Tracking** — one Notion Panels page per physical panel (notes in the page **body**, not the
  Description property; the *Armament Panel* page is the format template). Advance Status at each
  gate. Open Tasks/issues for prerequisites and deferred items.
- **NODE_ID** — claimed per controller in `Firmware/NODE_IDS.md` at A2.
- **Repo mirror** — condensed per-panel record in `docs/_source/controllers/<Panel>.md`.
- **Never** auto-merge PRs or place orders — the human owns B7–B9 and all merges.

## Delegates to

`panel-mapping` (B1) · `pcb-design` (B2, B5) · `cad` (B3) · `firmware` (B6) · `verify` (B9).
