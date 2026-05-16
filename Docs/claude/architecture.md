# Repository Structure

Organized by discipline, then by console position (Left / Center / Right):

- `CAD/` — Fusion 360 source files (`.f3d`). STLs and STEP exports are gitignored; generate from source.
- `PCB/` — KiCad projects. `PCB/<Console>/<Controller>/` holds one KiCad project per physical PCB. `PCB/Libraries/` holds shared symbols and footprints.
- `Firmware/` — PlatformIO projects (preferred) or Arduino sketches. Each subfolder is one STM32 controller. `Firmware/Libraries/` holds shared code used across controllers.
- `Docs/References/` — cockpit photos, manuals, screenshots. `Docs/Datasheets/` — component datasheets.
- `docs/claude/controllers/` — per-controller reference docs (pinout, I²C addresses, CAN IDs).

# Firmware Architecture

Each folder under `Firmware/` maps to one STM32 MCU board. Controllers communicate over CAN bus. A controller may drive one panel or a group of adjacent panels.

**Toolchain:** PlatformIO preferred (`platformio.ini` + `src/main.cpp`). Arduino IDE + STM32duino is an acceptable fallback (`.ino` file).

**I/O expansion:** MCP23017 (I²C, up to 8 per bus at addresses 0x20–0x27, 16 GPIO each)

**Analog inputs:** Resistor-ladder rotary selectors and pots read via STM32 ADC or I²C ADC (ADS1115) on breakout boards.

**Inter-board harness:** Breakout panel boards connect to their parent controller via a standardised 6-pin Molex MicroFit 3.0 harness:

| Pin | Signal |
|---|---|
| 1 | SDA |
| 2 | SCL |
| 3 | GND |
| 4 | GND |
| 5 | 12 V (LED backlight, PWM) |
| 6 | 3.3 V or 5 V (chip power — TBD per board) |

**Gauges:** X27.589 Switec stepper motors mounted shaft-through-PCB on the controller board.

**LED backlighting:** LEDs placed on the front side of the PCB; all other components on the back side.

**DCS communication:** TBD — evaluating DCS-BIOS over CAN gateway vs. direct USB HID.

**Naming convention:** Functional names from the start — `Center_Armament`, `Left_ECM`, etc. No `Controller_NN` placeholders.

**Licensing:** GPL v2 (`Firmware/LICENSE`) due to DCS-BIOS dependency (if used).

# PCB Architecture

Each physical PCB is its own KiCad project. Controller groups live under a shared parent folder:

```
PCB/<Console>/<ControllerGroup>/
├── <ControllerGroup>_MCU/    ← main board (MCU + panel switches/LEDs merged)
├── <Panel_A>/                ← breakout board, harness to MCU board
└── <Panel_B>/                ← breakout board, harness to MCU board
```

Main boards have LEDs on the front side and all other components on the back side.
