Scaffold a new OpenSkyhawk KiCad project. The working directory is the project root.

## Arguments

`$ARGUMENTS` format: `<Console> <Group> <BoardName> [mcu|breakout]`

| Arg | Valid values | Example |
|-----|-------------|---------|
| Console | `Center_Console`, `Left_Console`, `Right_Console` | `Center_Console` |
| Group | Controller group folder name | `Center_Armament` |
| BoardName | Board folder and file base name | `AWRS_Panel` |
| Type | `mcu` or `breakout` (default: `breakout`) | `breakout` |

**MCU board** — STM32F103CBT6 host board with CAN, power supply, and panel switches merged.
**Breakout board** — sub-panel board with MCP23017/ADS1115, harness connector to MCU board.

## Steps

### 1. Parse and validate

Split `$ARGUMENTS` on whitespace: Console=word1, Group=word2, BoardName=word3, Type=word4.
If Type is missing or empty, default to `breakout`.

Validate Console — must be one of `Center_Console`, `Left_Console`, `Right_Console`.
If invalid, stop and print: `Error: Console must be Center_Console, Left_Console, or Right_Console. Got: "<value>"`.

### 2. Resolve paths

- `BOARD_DIR` = `PCB/<Console>/<Group>/<BoardName>`
- `CONSOLE_DIR` = `PCB/<Console>`
- `GROUP_DIR` = `PCB/<Console>/<Group>`

Check `CONSOLE_DIR` exists — it must (all three console directories are committed). If missing, stop with a clear error.

Check `BOARD_DIR` — if it already exists AND contains files (not just .gitkeep), stop with:
`Error: Project already exists at <BOARD_DIR> — refusing to overwrite.`

Create `GROUP_DIR` if it doesn't exist: `mkdir -p "<GROUP_DIR>"`.

### 3. Create project directory

```bash
mkdir -p "<BOARD_DIR>"
```

### 4. Copy shared library and design-rule files

```bash
cp "PCB/Libraries/project-template/sym-lib-table" "<BOARD_DIR>/sym-lib-table"
cp "PCB/Libraries/project-template/fp-lib-table"  "<BOARD_DIR>/fp-lib-table"
cp "PCB/Libraries/design-rules/jlcpcb-standard.kicad_dru" "<BOARD_DIR>/jlcpcb-standard.kicad_dru"
```

The lib tables resolve to `${KIPRJMOD}/../../../Libraries/` — always 3 levels up to `PCB/Libraries/`.
The `.kicad_dru` file contains JLCPCB custom DRC rules (copper-to-edge, silkscreen, hole-to-hole, CAN pair).
To activate: PCB Editor → **File → Board Setup → Design Rules → Custom Rules** → paste file contents.

### 5. Generate root schematic UUID

```bash
python3 -c "import uuid; print(uuid.uuid4())"
```

Capture the output as `ROOT_UUID`.

### 6. Write `.kicad_pro`

Read `PCB/Libraries/project-template/template.kicad_pro`.
Replace every literal string `PROJECT_NAME` with `<BoardName>` and every literal string `ROOT_UUID` with the generated UUID.
Write the result to `<BOARD_DIR>/<BoardName>.kicad_pro`.

### 7. Write minimal root `.kicad_sch`

Get today's date as `YYYY-MM-DD`.
Write `<BOARD_DIR>/<BoardName>.kicad_sch` with the exact content below, substituting `ROOT_UUID`, `BoardName`, and `YYYY-MM-DD`:

```
(kicad_sch
  (version 20260306)
  (generator "eeschema")
  (generator_version "10.0")
  (uuid "ROOT_UUID")
  (paper "A4")
  (title_block
    (title "BoardName")
    (date "YYYY-MM-DD")
    (rev "1.0")
    (company "OpenSkyhawk")
  )
  (lib_symbols)
  (sheet_instances
    (path "/"
      (page "1")
    )
  )
)
```

### 8. Ensure shared designs library directory exists

```bash
mkdir -p "PCB/Libraries/sheets"
```

List `.kicad_sch` files in `PCB/Libraries/sheets/`. These are the available standard block templates.

### 9. Update the GitHub Project (panel tracking)

Find the panel's item in Project **#1 Panel Research & Assignment** (org `OpenSkyHawk`) — match
`<BoardName>` or a close variant (e.g. "AWRS Panel" for `AWRS_Panel`):
`gh project item-list 1 --owner OpenSkyHawk --format json`.

- **If found:** append the repo path + scaffolding status to the item body (`gh project item-edit --id <DI_…> --body ...`). Do NOT create a duplicate. (See the `panel-pipeline` skill for fields + the controller-graduation flow.)
- **If not found:** add a draft item (`gh project item-create 1 --owner OpenSkyHawk --title "<BoardName>"`) and set its `Console` / `Controller` fields.

Body note to add (substitute actual values):
```
PCB: `PCB/<Console>/<Group>/<BoardName>/` — KiCad scaffolded, schematic not yet started.
```

### 10. Report

Print a clean summary with these sections:

**Created**
List each file created with its full relative path.

**Library links**
- OpenSkyhawk symbols: `${KIPRJMOD}/../../../Libraries/OpenSkyhawk.kicad_sym` (via sym-lib-table)
- OpenSkyhawk footprints: `${KIPRJMOD}/../../../Libraries/OpenSkyhawk.pretty` (via fp-lib-table)
- Shared sheet templates: `PCB/Libraries/sheets/` — list .kicad_sch files found, or "none yet"

**Design rules loaded**
- Min trace: 0.2mm | Min clearance: 0.2mm | Min via drill: 0.3mm / pad: 0.56mm | Copper-to-edge: 0.3mm
- Predefined track widths: 0.2mm (signal), 0.3mm (LED feeds), 0.5mm (power), 1.0mm (12V)
- Predefined via sizes: 0.3mm/0.6mm (standard), 0.4mm/0.8mm (power)
- Custom DRC rules: `jlcpcb-standard.kicad_dru` (activate via Board Setup → Custom Rules)

**Next steps — MCU board** (if Type == mcu):
1. Open `<BoardName>.kicad_pro` in KiCad
2. Add hierarchical sub-sheets: Power, MCU (STM32F103CBT6), CAN (SN65HVD230), I/O (one sheet per MCP23017 instance), Backlighting (one per LED zone)
3. Refer to `docs/_source/hardware-standards.md` Standard Circuit Blocks section for each block's component list
4. Import templates from `PCB/Libraries/sheets/` via Place → Add Sheet when available
5. Run ERC (`kicad-cli sch erc`) after wiring each sheet

**Next steps — Breakout board** (if Type == breakout):
1. Open `<BoardName>.kicad_pro` in KiCad
2. Add hierarchical sub-sheets: Controls (MCP23017 + harness connector J1) and Backlighting (LED strings + Mini-Fit Jr J_LED)
3. Check `docs/_source/controllers/<Group>.md` for the I²C address and GPIO allocation for this board
4. Import templates from `PCB/Libraries/sheets/` via Place → Add Sheet when available
5. Run ERC (`kicad-cli sch erc`) after wiring each sheet
