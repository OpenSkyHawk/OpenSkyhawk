# KiCad Libraries

## Shared library setup

All projects use `PCB/Libraries/` as the single source of truth for custom components.
Library tables use `${KIPRJMOD}` — a built-in KiCad variable that always resolves to the project folder. **No global KiCad configuration required.** Anyone who clones the repo and opens a project gets the library automatically.

**For each new KiCad project — copy the template lib tables:**

```bash
cp PCB/Libraries/project-template/sym-lib-table PCB/<Console>/<Group>/<Board>/
cp PCB/Libraries/project-template/fp-lib-table  PCB/<Console>/<Group>/<Board>/
```

The tables resolve to `${KIPRJMOD}/../../../Libraries/` — always 3 levels up from the board folder to `PCB/Libraries/`. This matches the fixed project structure:

```
PCB/
├── Libraries/          ← shared library root
└── <Console>/
    └── <Group>/
        └── <Board>/   ← ${KIPRJMOD} (3 levels below Libraries)
            ├── sym-lib-table
            └── fp-lib-table
```

## Library contents

`PCB/Libraries/OpenSkyhawk.kicad_sym` + `PCB/Libraries/OpenSkyhawk.pretty/`

### Custom library (`OpenSkyhawk.kicad_sym` + `OpenSkyhawk.pretty/`)

| Symbol | Footprint | Status | Notes |
|--------|-----------|--------|-------|
| `OpenSkyhawk:LED_5050_Red` | `OpenSkyhawk:LED_5050_Red` | Ready | 6-pad RGB 5050 package, all R/G/B tied together as single colour. K=pad1 (right, x=2.4), A=pad2 (left, x=-2.4). Symbol matches standard `Device:LED` convention (K=pin1, A=pin2). Polarity marker (silkscreen triangle) is on the pad2/anode side — **the notched/chamfered corner of this specific LED is the anode** (verified with multimeter). Place LED with notch at the marker. 3D model from KiCad default lib. |
| `OpenSkyhawk:X27.589_Stepper` | `OpenSkyhawk:X27.589_Stepper` | Ready | From MH_Motors:X27-589. Polygon body outline. Shaft NPTH 4.6 mm at (0,−6), 4 NPTH mounts. 3D model: `OS_3DModels/x27168.step`. |
| `OpenSkyhawk:X27.168_Stepper` | `OpenSkyhawk:X27.168_Stepper` | Ready | From MH_Motors:X27-168. Circle body outline. Shaft clearance 4 mm at (0,−10.2), 1 NPTH mount. Same 3D model: `OS_3DModels/x27168.step`. |
| `OpenSkyhawk:DRV8835` | — | ❌ Removed | DRV8835 is only available in WSON-12 (bottom-terminated, not inspectable). Use `Driver_Motor:DRV8833PW` (KiCad built-in) instead — same capability, HTSSOP-16 package. |
| `OpenSkyhawk:AP63205WU` | `OpenSkyhawk:AP63205WU` | Pending | 12V→5V switching buck, SOT-23-6. Not used on standard MCU/breakout boards — only for future high-5V-current boards. Add symbol + footprint when needed. |
| `OpenSkyhawk:SN65HVD230` | — | N/A | Use KiCad built-in `Interface_CAN_LIN:SN65HVD230` directly — no custom entry needed. |
| `OpenSkyhawk:IRLML2502` | `Package_TO_SOT_SMD:SOT-23` | Ready | N-MOSFET LED zone switch (low-side), SOT-23, 20V/4A, logic-level gate (Vgsth 0.3–0.7V). Uses KiCad built-in SOT-23 footprint. Gate HIGH = on. |

### KiCad built-in libraries (no custom entry needed)

| Component | Library reference |
|-----------|------------------|
| STM32F103CBT6 | `MCU_ST_STM32F1:STM32F103CBTx` |
| AMS1117-3.3 | `Regulator_Linear:AMS1117-3.3_SOT223` |
| MCP23017 | `Interface_Expansion:MCP23017x-x-SO` (SOIC-28) |
| DRV8833PW | `Driver_Motor:DRV8833PW` (HTSSOP-16) |
| ADS1115 | `Analog_ADC:ADS1115` |
| Crystal 8 MHz | `Device:Crystal` |
| JST-XH connectors | `Connector_JST:JST_XH_*` |
| Molex Mini-Fit Jr (main bus) | `Connector_Molex:Molex_Minifit_Jr_5557-*` (dual-row) |
| Molex Mini-Fit Jr (LED power, 2-pin) | `Connector_Molex:Molex_Mini-Fit_Jr_5566-02A2_2x01_P4.20mm_Vertical` |

## Adding new components

1. Open `PCB/Libraries/OpenSkyhawk.kicad_sym` in the Symbol Editor
2. Add the symbol; set Footprint to `OpenSkyhawk:<name>`
3. Create the matching `.kicad_mod` in `PCB/Libraries/OpenSkyhawk.pretty/`
4. Commit both files — no duplication across projects needed

## Shared hierarchical sheet templates

`PCB/Libraries/sheets/` — reusable `.kicad_sch` files for standard circuit blocks (power rail, MCP23017 instance, LED zone switch, ADC filter, CAN transceiver).

To use in a project: **Place → Add Sheet** in KiCad, browse to `PCB/Libraries/sheets/<name>.kicad_sch`.

See `PCB/Libraries/sheets/README.md` for the list of available templates and how to add new ones.

## Design rules

JLCPCB standard 2-layer constraints and predefined sizes are documented in `docs/claude/pcb-design-rules.md`.

They are pre-loaded into every new project via `template.kicad_pro` (board design settings) and `jlcpcb-standard.kicad_dru` (custom DRC rules). See that doc for the full constraint table and JLCPCB ordering settings.

## Scaffolding new projects

**Whenever a new PCB board needs to be created, always invoke the `/new-kicad-project` skill. Do not manually create project files.** This is the required method — not optional.

Triggers: user asks to "create", "add", or "scaffold" a new board, panel, or KiCad project.

```
/new-kicad-project <Console> <Group> <BoardName> [mcu|breakout]
```

Example: `/new-kicad-project Center_Console Center_Armament AWRS_Panel breakout`

The skill creates and then updates Notion:
- `PCB/<Console>/<Group>/<BoardName>/sym-lib-table` + `fp-lib-table` (copied from template)
- `<BoardName>.kicad_pro` (with OpenSkyhawk libs pinned, correct ERC severities)
- `<BoardName>.kicad_sch` (minimal root sheet with title block)
- Finds the existing Notion Panels page (or creates one) and adds the repo path to the page body

---

# KiCad CLI

Path: `/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli` (v10.0.1)

```bash
KICAD=/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli

# Validation
$KICAD pcb drc --output drc.json <board.kicad_pcb>
$KICAD sch erc --output erc.json <schematic.kicad_sch>

# Fabrication
$KICAD pcb export gerbers --output ./gerbers/ <board.kicad_pcb>
$KICAD pcb export drill --output ./gerbers/ <board.kicad_pcb>
$KICAD sch export bom --output bom.csv <schematic.kicad_sch>

# Fusion 360 fit check
$KICAD pcb export step --output board.step <board.kicad_pcb>

# Panel cutout template
$KICAD pcb export dxf --output cutout.dxf <board.kicad_pcb>

# Documentation
$KICAD sch export pdf --output schematic.pdf <schematic.kicad_sch>
```
