---
name: panel-pipeline
description: Orchestrate building an OpenSkyhawk cockpit controller end-to-end — research → schematic → CAD → backlight → PCB → firmware → order → assembly → test. Use when starting a new panel or controller, advancing one to its next stage, resuming a partially-built one (detect where it stands and continue), or partitioning the cockpit's panels into controllers. A thin orchestrator: it sequences stages, enforces gates, keeps the GitHub Projects + repo tracking + InvenTree inventory/cost in sync, owns the cross-cutting decisions (controller grouping, NODE_ID, prerequisites), and delegates the discipline work to panel-mapping, pcb-design, cad, firmware, and verify. Multi-session and resumable.
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

Read the panel/controller's **GitHub Project status** (the tracker) and cross-check **repo signals**:

| Stage reached | Repo signal |
|---|---|
| Grouped | NODE_ID claimed in `Firmware/NODE_IDS.md` |
| Researched | `docs/_source/controllers/<Panel>.md` exists + Project item research complete |
| Schematic | KiCad project exists + ERC clean |
| CAD | `.FCStd`/`.f3d` source exists |
| Backlight | LED-array count set in schematic + re-ERC clean |
| PCB | routed + DRC clean + gerbers exported |
| Firmware | `Firmware/Panels/<Controller>/` compiles + tests pass |
| Ordering / Assembly / Testing | Project Status + order ref / boards in hand |

If the Project and the repo disagree, trust the repo signal and correct the Project. *(Conflict-resolution
is a thin detail — refine in use.)*

## Scope: controller, not panel

A **controller** = one STM32 PanelGroup MCU = one CAN **NODE_ID**, hosting a **host panel + I²C
breakouts within ~12–18"**. GitHub Project #1 keeps **one item per physical panel**; the build pipeline runs
**per controller** (Project #2). The grouping rule + budgets are summarized in A2 below.

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

Output: per-panel I/O + actuator budget, recorded in the panel's GitHub Project item. Also run a **bulk
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

**Backend / interface class per sub-panel** (#197 decision — hardware-standards →
Shift-Register I/O): panels with **rotary encoders or fast gauges (≳150 °/s)** → SPI-class
(74HC ShiftBus J_SR leg, `SHIFTBUS_ISR_HZ=1000`; one end-node leg per host chain); everything
else → I²C-class (MCP/OLED/ADS — internals free, the connector is the standard). Weigh
**OLED + fast-gauge co-residency** on one node: a display flush stalls same-node stepper slews
regardless of backend (+35 % w/ jitter at a simulated 25 ms flush); slow rare slews (APN-153
DRIFT) accepted.

Bias to **split** — up to 63 CAN nodes are essentially free. Grouping is **provisional here**
(mod-file distances) and **confirmed at B3 CAD** against the real harness path (re-split if a
breakout exceeds 18"). Claim each NODE_ID in `Firmware/NODE_IDS.md`.

#### PanelGroup base — fixed resources (cache; full ref: `docs/_source/base-boards.md`)

Every controller starts from the **PanelGroup base** (STM32F103C8T6, LQFP48). The base fixes
these — don't re-derive, build the per-controller budget on top:

- **Reserved (not available):** CAN PA11/PA12 · diag USART1 PA9/PA10 · SWD PA13/PA14 ·
  status LED PB14(red)/PB15(grn) · backlight PWM PA6(BL1)/PA7(BL2) · I2C1 PB6/PB7 ·
  I2C2 PB10/PB11 · I2C1 INT PB12/PB13 · I2C2 INT PB8/PB9 · OSC PD0/PD1 · NC PC13/14/15 + PB2.
- **Breakout (free panel I/O) — 13 GPIO:** ADC+PWM `PA0 PA1 PA2 PA3 PB0 PB1` · ADC-only
  `PA4 PA5` · PWM/digital (JTAG-remap for PA15/PB3/PB4) `PA8 PA15 PB3 PB4 PB5`. **8 ADC-capable.**
- **I²C:** 2 buses, each with INT_A/INT_B brought to its 8-pin JST-XH header → **one INT-driven
  MCP23017 (inputs) per bus**; output-only expanders need no INT. Addressing MCP23017 0x20–0x27,
  ADS1115 0x48–0x4B per bus. Prefer a **direct STM32 ADC breakout pin** over an ADS1115 when only
  1–2 analog channels are needed.
- **Backlight:** 2 PWM-dimmed low-side zones on-base (sized at B4).

---

## A2b — Graduate to GitHub issues · *owner: AI*

Tracking lives in two **private** GitHub Projects (org `OpenSkyHawk`): **git is the data source
of truth, the Projects are the generated tracker.** Get the field/option node IDs live with
`gh project field-list <n> --owner OpenSkyHawk --format json`.

- **#1 Panel Research & Assignment** — every panel as a **draft** through A1/B1 (fields: Console,
  #Controls, Breakdown, Panel Type, Priority, + independent flag columns `Controls?` /
  `Screenshot?` / `Analysis?`). `Analysis?=Done` only after a ModelViewer screenshot is reviewed
  **and** the input list verified — it's downstream of `Screenshot?`, never auto-set from migrated data.
- **#2 Controller Build** — each controller as an **issue** through B1–B9.

**Graduate a controller** once A2 groups its panels (draft→issue conversion is **one-way**, and
issues are **public** since the repo is public — the Projects stay private via their own ACL):

1. Create the **controller** issue — label `controller`, add to #2, set build `Status`. Body =
   member panels + NODE_ID + I/O architecture. Each controller step (B6 Firmware · host/MCU PCB ·
   B7 Order · B8 Assembly · B9 Integration test) is its own **sub-issue** (label `build-step`)
   under the controller, created in build order.
2. For each member panel: **convert** its #1 draft → issue (`convertProjectV2DraftIssueItemToIssue`),
   label `panel`, keep it in #1 *and* add to #2, then **link as a sub-issue** of the controller (`addSubIssue`).
3. Panel issue body = its control inventory (B1 / `panel-mapping` output). Each panel step
   (B2 Schematic · B3 CAD · B4 Backlight · B5 PCB) is its own **sub-issue** (label `build-step`)
   under the panel, created in build order.
4. **Close** a step sub-issue when its PR merges (link with `Closes #<step>`); advance the
   controller `Status` as steps complete. All panel sub-issues closed = hardware done → finish
   group firmware + test.

**Sub-tasks are defined by issue type:** `controller` → firmware/test/order/assembly · `panel` →
schematic/PCB/CAD/backlight. **Default is 3-level** — every build step is its own **sub-issue**
(label `build-step`; controller steps under the controller, panel steps under the panel; GitHub
nests ≤8 deep), giving assignable, PR-closable steps + fine progress roll-up on the parent.
**2-level** (steps as body checkboxes, no step issues) is the lighter fallback for a quick or
low-priority controller. **Step sub-issues are NOT added to the Projects board** — they roll up
under their parent; only controllers (#2) + panels (#1 + #2) sit on the board. The **checkpoint**
is the `Status` field; reserve **Milestones** for coarse gates (a console, a release). Issue
templates have no per-template ACL, so this skill (not a template) owns the structure.

---

## Phase B — per-controller build (repeats per controller)

### B1 — Research (deep) · `panel-mapping` · *AI drafts → human photos + in-sim type confirm*

A1 gave the type *estimate*; B1 makes it build-ready — **AI drafts from the A4EC Lua
(`clickabledata.lua` gestures) + DCS-BIOS + the A1 analysis; human confirms each control in the
real sim.** Deliverables:

- **Interaction model — per control, sim-confirmed.** *How the pilot operates it*, beyond the type:
  physical action (toggle up/down · push momentary/latching · rotate detented-N-pos / continuous /
  multi-turn · pull-to-unlock · **concentric** inner/outer · **guarded** · **spring-return**), exact
  **positions + labels**, detent feel, special behaviours (push-to-set, spring-loaded slew,
  lift-lock). Drives part selection + confirms the FW class. Record as an **`Interaction`** column in
  the Inputs table (the *how*, beside FW class = the *what*).
- Confirm control **types in the real sim**; capture reference photos (human). **Dimensions** (×1.10)
  + switch **centre-to-centre spacing** → CAD. **Part selection** per control.
- **Engineering pass (controller-wide):**
  - **Pin budget** — tally against the base (13 breakout GPIO / 8 ADC-capable · 2 I²C buses, ≤8
    MCP23017 & ≤4 ADS1115 per bus). Confirm it fits with headroom.
  - **Gauge type + drive** — classify every gauge: **DrumDisplay = OLED** (I²C, cheap buffer write)
    vs **NeedleGauge = stepper / DRV8833** (needs fast coil drive). **MCP23017-driven steppers are
    I²C-rate-limited** (~490 steps/s @400 kHz, *shared per bus*) — default to MCP for low/slow gauge
    counts, but **bench-validate the real step rate + smoothness before B5** (hard gate; don't lay
    out copper on an unproven rate). Fast / dense clusters → direct MCU GPIO or the **74HC595
    shift-register backend (#133)**. **DrumDisplay = OLED supersedes any motorized-drum assumption —
    recount expanders / steppers on that basis** (it usually collapses them).
  - **Inter-panel connections** — per host↔sub-panel link: I²C harness (8-pin JST-XH:
    SDA/SCL/INT_A/INT_B/+3V3/GND/spare-analog), analog passthrough → host ADC, what stays **local**
    (stepper coils + local MCP — don't cross the harness), backlight +12 V, power.
  - **Pin map per MCU (NODE)** — the authoritative table B2 consumes: every STM32 pin → function,
    I²C addressing (MCP23017 0x20–0x27/bus, OLED mux channels), INT routing.
- **Gap analysis** — for each control type, check the OpenSkyhawk building block exists: **both** the
  **firmware class** (e.g. `OpenSkyhawk::Switch3Pos`) **and** the **KiCad symbol / Design Block**.
  Each missing piece → a **per-board prerequisite ticket** (blocker on this controller): missing
  **class** blocks **B6**, missing **symbol** blocks **B2**. **Building a new control type is OUT OF
  SCOPE here** — its own firmware / KiCad effort (TechSpec + prototype, e.g. how `Switch2Pos` was
  built); the pipeline flags, tickets, and blocks until it lands. Implemented today: `Switch2Pos ·
  Switch3Pos · SwitchMultiPos · AnalogMultiPos · AnalogInput · RotaryEncoder · LED · DrumDisplay
  (OLED) · NeedleGauge · PinRef`. Missing/planned: `ActionButton · AngleSensor · SwitchWithCover2Pos
  · ServoMotor (#132)`; fast-coil backend `74HC595 (#133)`. **Confirms the A1 provisional
  Ready/Blocked** now that types are sim-verified.
- **Body cleanup** — refine the A1 first-pass into build-ready issue bodies; **preserve Notes +
  Screenshot verbatim** (humans own those).
- Out: Controls Inventory + **Interaction** + I/O Summary + Dimensions + **Pin map** + prerequisites
  → Project item / issue body + `docs/_source/controllers/<Panel>.md`.

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
- **BOM → InvenTree** — create/confirm each part (+ supplier part) and build the panel **Assembly**'s
  BOM (see *Inventory & cost tracking*).
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
  Mirror each order (JLC / LCSC) as an InvenTree **Purchase Order** — part lines + shipping/tax as
  extra lines; **receive** on arrival → stock at landed cost (see *Inventory & cost tracking*).
- **B8** — solder (T962 reflow) + install + harness.
- **B9** — flash + verify each control in DCS (inputs register, gauges/backlight move, CAN reaches
  CONNECTED). AI supplies the checklist (`verify`). → **Done**.

---

## Inventory & cost tracking (InvenTree)

Components, BOMs, and orders are tracked in **InvenTree** (self-hosted, `http://192.168.85.85`, LXC
on pve2) — the **system of record for inventory + cost**. Goal: per-panel **cost-to-source** and,
with labor + machine time, a **sell price** (panels sold **assembled**). This skill wires panel
parts / BOMs / orders into InvenTree; it does **not** duplicate it. Live state, IDs, and API gotchas
live in memory `project_inventree`.

**Boundary — InvenTree is downstream of design.** Part selection (B1–B5) is **engineering-driven**;
stock / availability **never** drives what part you pick. InvenTree consumes the *finalized* BOM —
the dependency flows one way (design → BOM → InvenTree, never back). The only feedback loop is
*"can I build N now?"* (Build Orders vs stock — a production question, not a design one). The
committed KiCad project stays **self-contained on `PCB/Libraries/`** (LCSC / MPN as symbol fields);
InvenTree is a **decoupled downstream consumer**, never referenced in the project's lib tables (the
repo is public — no live link to the private instance).

- **3-layer part identity:** Part → **ManufacturerPart** (real maker + MPN) → **SupplierPart**
  (LCSC / JLCPCB + SKU + price). LCSC/JLC are *distributors*, not makers.
- **Categories:** `Electronics{Passives,Semiconductors,Connectors,Modules,Crystals}` · `Controls`
  (HMI: switches / encoders / pots / buttons / indicators) · `Fabricated{3D-Printed,Laser-Cut,UV}` ·
  `RawMaterial` (units: filament g / resin mL / sheet cm² / ink mL) · `Fasteners` · `Bare PCB` ·
  `Assemblies`.
- **A panel = an InvenTree Assembly** whose BOM = the PanelGroup base assembly + panel-specific
  HMI / fabricated / fastener parts + its bare PCB. Multi-level → per-panel cost rolls up.
- **Touchpoints:**
  - **B5 (post-BOM)** — push the *finalized* KiCad BOM → InvenTree (CSV import, or `inventree-part-import`
    for LCSC parts — see **LCSC import** below) → creates the parts (HMI → `Controls`, etc.) + the panel
    **Assembly**'s BOM. **Decoupled push, not a live KiCad link.** Per-panel part list also derivable from
    the control inventory (`docs/_source/a4ec-control-inventory.csv`) via FW-class → physical-part map.
  - **B7** — each LCSC / JLC order → a **Purchase Order** (part lines + shipping/tax as *extra
    lines*); **receive** on arrival → stock at landed cost.
  - **Cost → price** — BOM rollup = material; add **`Assembly Labor (hr)` + `Machine time` virtual
    BOM parts** (priced per the build's labor/machine rates) → fully-loaded unit cost → set the
    assembly's **Sale Price** → margin. A **Build Order** is the production run (consumes component
    stock, outputs the assembly at build cost).
- **Access:** API token at `~/.config/inventree/token` (read it, never commit/echo). Small LXC →
  **retry + backoff on HTTP 500** (DB-lock) for bulk writes.
- **LCSC import — `inventree-part-import`** (invoke `~/ipi-venv/bin/python -m inventree_part_import`; v1.10.0
  with LCSC API v3 — no console script, so use `-m`). Pulls a part's full data (description, parameters,
  **image**, price breaks) straight from an LCSC part number. Use at **B5** (seed new BOM parts) / **B7**
  (order-time price refresh):
  - **First-time setup = the tool's OWN `--configure lcsc` wizard.** Never hand-write its
    `inventree.yaml` / `suppliers.yaml` internals.
  - **Import NEW parts by C-number:** `~/ipi-venv/bin/python -m inventree_part_import -i false -o lcsc C1234 C5678 …`
    (`-i false` = non-interactive/safe; `-d` = dry-run, *not* allowed with `--update`). Run once with `-i true`
    when a new LCSC category needs mapping to your tree — the answer is saved to `categories.yaml`. Declare new
    subcategories there with `_aliases` (LCSC category strings) → the tool auto-creates them on the next run.
  - ⚠️ **NEW parts ONLY.** It matches an existing InvenTree part by **`name == MPN`** — OpenSkyhawk parts use
    *descriptive* names (e.g. `R 1k 0.1% 0805`), so a re-import creates an **MPN-named duplicate** instead of
    updating (`--update CAT` searches by name too → same trap). To refresh an existing descriptively-named part
    (image/description), attach the data **directly via the API**, not through import.
  - Config dir `~/Library/Application Support/inventree_part_import/`. Full workflow + the duplicate
    merge-repair pattern (reassign SupplierPart/ManufacturerPart → original, deactivate-then-delete the dup)
    live in memory `project_inventree`.

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

- **Tracking** — one GitHub Project item per physical panel (Project #1; controller issue in #2),
  notes in the item/issue **body**. Advance `Status` at each gate. Open GitHub issues for prerequisites and deferred items.

- **Panel item / issue body structure** (in this order, all sections present):
  1. **Header line** — auto-generated control count summary (`**N controls** — X inputs · Y gauges · Z LEDs`)
  2. **Notes** — human-authored observations: physical layout, construction decisions, open questions. Never overwrite.
  3. **Screenshot** — embedded image tag(s). Manual scans or ModelViewer shots. Never overwrite. Screenshots are attached directly to GitHub issues/drafts — do NOT commit images to the repo.
  4. **Prerequisites** — AI-generated checklist of missing `OpenSkyhawk::` firmware classes / KiCad symbols, each linked to its GitHub issue. One line per blocker.
  5. **Research checklist** — three standard items (A-phase only): `- [x] Control list identified (Lua + DCS-BIOS)` / `- [ ] Reference screenshot attached to ticket` / `- [ ] Assigned to a controller`. Coords + dimensions are B3 (CAD), not tracked here.
  6. **Analysis notes** — AI-generated prose: confirmed control types, physical layout findings, design decisions, open questions.
  7. **Inputs** — table: `Control | Identifier | DCS-BIOS addr | FW class | Notes`
  8. **Outputs** — table: `Display group | Identifiers | Digits | Flag | FW class`
  9. **Component Summary** — AI-generated I²C device + discrete-driver + STM32-direct-pin tables for the panel's own I/O (parts + LCSC + addresses/pins). Per-panel scope; the controller-wide budget + remaining-pin availability lives on the controller issue, not here. Each part is tracked in **InvenTree** (Part + SupplierPart + price); the panel itself is an InvenTree **Assembly** with a BOM — see *Inventory & cost tracking*.

  When updating a ticket: **always fetch the current body first**, extract Notes + Screenshot verbatim, then rebuild as fetched-Notes + fetched-Screenshot + updated AI sections. **Never reconstruct from memory or session context** — the user edits Notes and Screenshot directly in the GitHub UI, and overwriting them destroys their work irreversibly.
- **NODE_ID** — claimed per controller in `Firmware/NODE_IDS.md` at A2.
- **Repo mirror** — condensed per-panel record in `docs/_source/controllers/<Panel>.md`.
- **Never** auto-merge PRs or place orders — the human owns B7–B9 and all merges.

## Delegates to

`panel-mapping` (B1) · `pcb-design` (B2, B5) · `cad` (B3) · `firmware` (B6) · `verify` (B9).
