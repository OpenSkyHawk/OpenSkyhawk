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
