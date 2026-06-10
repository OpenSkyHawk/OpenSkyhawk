# AGENTS.md

This file provides guidance to Codex (Codex.ai/code) when working with code in this repository.

> **Note:** This file is a standalone summary for Codex. The authoritative source is `CLAUDE.md` + `docs/claude/`. Keep in sync when updating hardware standards or architecture.

## Project

OpenSkyhawk is a physical DCS A-4E Skyhawk home cockpit build. It produces 3D-printed panels, custom PCBs, and STM32 firmware to replicate the full A-4E cockpit for use with the DCS A-4E Community Mod. Controllers communicate over CAN bus.

## Repository Structure

Organized by discipline, then by console position (Left / Center / Right):

- `CAD/` — Fusion 360 source files (`.f3d`). STLs and STEP exports are gitignored; generate from source.
- `PCB/` — KiCad projects. `PCB/<Console>/<Controller>/` holds one KiCad project per physical PCB. `PCB/Libraries/` holds shared symbols and footprints.
- `Firmware/` — PlatformIO projects (preferred) or Arduino sketches. Each subfolder is one STM32 controller. `Firmware/Libraries/` holds shared code used across controllers.
- `docs/References/` — cockpit photos, manuals, screenshots. `docs/Datasheets/` — component datasheets.
- `docs/claude/` — Claude Code reference docs (architecture, hardware standards, KiCad notes).

## Firmware Architecture

Each folder under `Firmware/` maps to one STM32F103CBT6 MCU board. Controllers communicate over CAN bus. A controller may drive one panel or a group of adjacent panels.

**Toolchain:** PlatformIO preferred (`platformio.ini` + `src/main.cpp`).

**MCU:** STM32F103CBT6 — LQFP48, 128 KB flash, 20 KB RAM. Requires external 8 MHz crystal for CAN. PA11/PA12 are shared between USB and CAN — pick one at firmware init.

**I/O expansion:** MCP23017 (I²C, up to 8 per bus at addresses 0x20–0x27, 16 GPIO each)

**Stepper driver:** DRV8835 (HTSSOP-16) drives X27.589 Switec stepper gauges. Use SwitecX25 library for homing. nSLEEP held LOW until DCS-BIOS sim connection established.

**Power:** 12 V → AP63205 (SOT-23-6 buck) → 5 V → AMS1117-3.3 (SOT-223 LDO) → 3.3 V

**DCS communication:** DCS-BIOS (evaluating CAN gateway vs. direct USB HID)

**Naming convention:** Functional names from the start — `Center_Armament`, `Left_ECM`, etc.

**Licensing:** GPL v2 (`Firmware/LICENSE`) due to DCS-BIOS dependency.

## PCB Architecture

Each physical PCB is its own KiCad project. Controller groups live under a shared parent folder:

```
PCB/<Console>/<ControllerGroup>/
├── <ControllerGroup>_MCU/    ← main board (MCU + panel switches/LEDs merged)
├── <Panel_A>/                ← breakout board, harness to MCU board
└── <Panel_B>/                ← breakout board, harness to MCU board
```

**Custom library:** `PCB/Libraries/OpenSkyhawk.kicad_sym` + `PCB/Libraries/OpenSkyhawk.pretty/`. Each new project gets a copy of `PCB/Libraries/project-template/sym-lib-table` and `fp-lib-table` — these use `${KIPRJMOD}` and require no global KiCad configuration.

**Shared sheet templates:** `PCB/Libraries/sheets/` — reusable hierarchical `.kicad_sch` blocks (power rail, MCP23017, LED zone, etc.). Import via Place → Add Sheet.

**Scaffolding:** use `/new-kicad-project <Console> <Group> <BoardName> [mcu|breakout]` to create a new project with library tables, `.kicad_pro` (JLCPCB design rules + net classes pre-loaded), minimal root schematic, and `jlcpcb-standard.kicad_dru` in one step.

**PCB design rules:** JLCPCB standard 2-layer. Min trace/clearance: **0.2 mm** (floor matches Default net class). Full rules in `docs/claude/pcb-design-rules.md`.

**Net classes (pre-loaded in every project):**
- `Default` — 0.2 mm signal traces
- `LED_Trunk` — 0.5 mm, auto-assigned to `+12V_BACKLIGHT` / `BACKLIGHT_SW_RETURN`
- `LED_String` — 0.3 mm, manual assignment for per-string traces
- `CAN` — 0.2 mm / 0.2 mm clearance, auto-assigned to `CANH` / `CANL`

**Board power budget:** logic + LED boards ≤ 500 mA at 12V input. Actuator boards (solenoids, servos, large steppers) require separate design review — see `docs/claude/pcb-design-rules.md`.

## Hardware Standards

**Package rule:** All ICs must be visually inspectable after reflow — SOIC, SSOP, TSSOP, HTSSOP, LQFP, SOT-23, SOT-223, through-hole. No QFN, DFN, BGA, or any fully-bottom-terminated package.

| Screw | Use |
|---|---|
| M2 | PCB mounts, small standoffs |
| M3 | Placards, light rings, small brackets |
| M4 | Instrument bezels, gauge mounts — clearance Ø4.3–4.5 mm |
| M5 | Panel-to-subpanel, corner mounts — clearance Ø5.3–5.5 mm |

Connectors: Molex Mini-Fit Jr (4.2 mm, CAN bus/power main bus) and JST-XH (2.54 mm, everything else). Minimum pitch 2.54 mm. Wire gauge 24 AWG throughout.

LED backlighting: 5050 SMD red, PCB front face, 12 V low-side PWM (IRLML2502 N-ch MOSFET, gate driven directly by STM32 3.3V PWM), 120Ω default per string (~18 mA), 5 LEDs in series per string. LED power on separate 2-pin Mini-Fit Jr connector (+12V_BACKLIGHT / BACKLIGHT_SW_RETURN); not carried on signal harness.

## KiCad CLI

Path: `/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli` (v10.0.1)

```bash
KICAD=/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli
$KICAD pcb drc --output drc.json <board.kicad_pcb>
$KICAD sch erc --output erc.json <schematic.kicad_sch>
$KICAD pcb export gerbers --output ./gerbers/ <board.kicad_pcb>
$KICAD sch export bom --output bom.csv <schematic.kicad_sch>
$KICAD sch export pdf --output schematic.pdf <schematic.kicad_sch>
```

## Licensing

| Layer | License |
|---|---|
| CAD, PCB, Docs | CC BY-NC-SA 4.0 (root `LICENSE`) |
| Firmware | GPL v2 (`Firmware/LICENSE`) |

## Git

- Remote: `git@github.com:mottihoresh/OpenSkyhawk.git`
- Do not add `Co-Authored-By:` trailers to commits
- STLs, Gerbers, and other generated outputs are gitignored — commit sources only
