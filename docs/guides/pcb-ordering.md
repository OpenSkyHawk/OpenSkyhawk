# PCB Ordering (JLCPCB)

OpenSkyhawk boards are designed for **JLCPCB's standard 2-layer service**. The design rules are
already tuned for it ([PCB Design Rules](../hardware/pcb-design-rules.md)); this guide is how to
get from a finished layout to an order.

## 1. Export the fab files

From a clean, DRC-passing layout, export Gerbers and drill data with the KiCad CLI:

```bash
KICAD=/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli

$KICAD pcb export gerbers --output ./gerbers/ <board.kicad_pcb>
$KICAD pcb export drill   --output ./gerbers/ <board.kicad_pcb>
```

Run DRC first — fix everything before ordering:

```bash
$KICAD pcb drc --output drc.json <board.kicad_pcb>
```

Zip the `gerbers/` directory. (Gerbers and drill files are generated output — **gitignored**,
not committed.)

## 2. Upload and configure

Upload the zip to JLCPCB. The cost-optimised settings, all under **High-Spec Options**:

| Option | Setting |
|--------|---------|
| Layers | 2 |
| Outer copper weight | 1 oz |
| Via covering | Tented |
| Min via | 0.3 mm |
| Outline tolerance | ±0.2 mm (Regular) |
| Electrical test | Flying-probe (included) |
| Mark on PCB | Remove mark |
| Confirm production file | No (faster lead time) |
| Gold fingers / castellation / edge plating | No |

These match the constraints the boards are designed to, so the order should pass review without
back-and-forth.

## 3. Assembly

Most OpenSkyhawk boards are hand-/reflow-assembled rather than ordered with JLCPCB SMT
assembly — the package choices are all reflow-and-inspect friendly by design. See
[Assembly & Soldering](assembly.md).

!!! note "Panel cutout template"
    Need a panel cutout to match the board outline? Export a DXF:
    `kicad-cli pcb export dxf --output cutout.dxf <board.kicad_pcb>`.
