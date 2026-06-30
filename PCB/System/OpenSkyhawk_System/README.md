# OpenSkyhawk — System Architecture

**Documentation only — NOT a fabricated board.** No PCB, no gerbers, no DRC. This KiCad
project is the **master cockpit system architecture**: a hierarchical, drill-down schematic
showing how every board, panel, bus, and harness in the cockpit connects.

It is the visual + electrically-checkable form of the system the GitHub Projects + repo track.
KiCad can't live-link separate projects, so this is the **interface contract**: harness pinouts
and bus structure defined once; each board's connector conforms.

## Hierarchy (drill-down)

Mirrors the repo (`PCB/<Console>/<Group>/<Board>`) and GitHub Projects (console → controller → panel):

```
ROOT — Full cockpit
  3 console sheets · SimGateway(RP2040) ↔ PanelBridge(STM32) ↔ PC · CAN backbone · power tree · notes
  ├─ Right_Console (sheet)
  │    └─ Right_Navigation (sheet) → host ASN-41 + APN-153 + ARC-51 symbols, pin-level harness
  │    └─ (future controllers…)
  ├─ Center_Console (sheet) → Center_Armament → …
  └─ Left_Console (sheet) → …
```

| Tier | Sheet | Shows |
|---|---|---|
| 1 Cockpit | root (`OpenSkyhawk_System.kicad_sch`) | consoles + gateway/bridge/PC + CAN + power tree + notes block |
| 2 Console | `Right_Console` / `Center_Console` / `Left_Console` | that console's controllers (panel groups) |
| 3 Controller | `Right_Navigation`, … | host board + panel **symbols**, harness wired pin-level |

Panels are **symbols on the tier-3 sheet** (not a 4th sheet tier) unless one is large.

## Conventions

- **Symbol per endpoint** — each host board, panel, and key component (MCP23017, TCA9548A, OLED,
  DRV8833, X27, STM32 host) is a KiCad symbol with its connector pins; shared from
  `Libraries/OpenSkyhawk.kicad_sym`, reused across sheets. New: panel symbols (ASN-41, APN-153, ARC-51).
- **CAN bus + power are electrically flat** → shown as **global nets / off-sheet connectors** passed
  down each tier (consoles are organizational drill-down, not separate buses).
- **Notes block** (root, top-left): wire gauge (20–24 AWG silicone), crimps (DLL-5556 / JST-XH),
  Molex Mini-Fit Jr bus, color scheme, termination; "parts + cost in InvenTree."
- **External connections** stub: CAN → PanelBridge · USB → PC · +12V/+5V power-in.
- Grows incrementally — add a console/controller sheet as each is built. Not linked to the board
  PCB projects (doc-only); per-controller harness detail lives here as sub-sheets.

## Build order (in KiCad — Add Sheet manages hierarchy + UUIDs safely)

1. Root: draw the 3 console hierarchical sheets + the SimGateway↔PanelBridge↔PC chain + CAN/power
   global labels + the notes block.
2. `Right_Console` sheet → add the `Right_Navigation` controller sheet.
3. `Right_Navigation` sheet → place host + 3 panel symbols, wire the harness (Bus A / Bus B nets
   from the controller B1: `SDA/SCL`, `SDA2/SCL2`, `INT`, `VOL`, `+5V/+3V3`, `GND`, 2 native).
4. Stub `Center_Console` / `Left_Console` + their controllers as the build progresses.

## Status

Scaffolded — root sheet only (A3, title block, notes TBD). Sub-sheets + symbols built in KiCad.
First populated controller = **Right_Navigation** (3 panels B1 ✓).
