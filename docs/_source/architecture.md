# Repository Structure

Organized by discipline, then by console position (Left / Center / Right):

- `CAD/` — source CAD models (`.f3d` or `.FCStd`; tooling under evaluation, Fusion 360 vs FreeCAD — see design decision D8). STLs and STEP exports are gitignored; generate from source.
- `PCB/` — KiCad projects. `PCB/<Console>/<Controller>/` holds one KiCad project per physical PCB. `PCB/Libraries/` holds shared symbols and footprints.
- `Firmware/` — PlatformIO projects (preferred) or Arduino sketches. `Firmware/<name>/` — one production firmware sketch per folder (STM32 or RP2040, flat — no MCU-type subdirectory). `Firmware/Libraries/` — shared code; STM32 and RP2040 libraries in separate subdirectories.
- `docs/References/` — cockpit photos, manuals, screenshots. `docs/Datasheets/` — component datasheets.
- `docs/_source/controllers/` — per-controller reference docs (pinout, I²C addresses, CAN IDs).

# Firmware Architecture

**Authoritative spec:** `Firmware/ScratchPad/FirmwarePlan/README.md`  
**Implementation specs:** `Firmware/ScratchPad/TechSpec/README.md`

Three-tier architecture: **SimGateway** (RP2040) ↔ **PanelBridge** (STM32, CAN master) ↔ **PanelGroup** nodes (STM32, CAN sub-nodes). The FirmwarePlan documents are split by contract boundary and are the source of truth for all firmware behaviour.

Key facts that affect PCB and hardware decisions:

- **MCUs:** STM32F103C8 (LQFP48) for CAN avionics nodes — STM32F103CB only where extra flash is needed (PanelBridge); RP2040 off-the-shelf modules for SimGateway and HID flight controls (no custom PCB for RP2040)
- **CAN transceiver:** SN65HVD230 on PA11/PA12 per STM32 board
- **NODE_ID:** assigned per board via `platformio.ini` `build_flags = -DNODE_ID=N`; visible to all translation units as a compile-time constant
- **Reserved STM32 pins:** see `FirmwarePlan/08-hardware-firmware-contracts.md`
- **Toolchain:** PlatformIO (`platformio.ini` + `src/main.cpp`); STM32duino Arduino-compatible framework
- **Naming:** functional names from the start — `Center_Armament`, `Left_ECM`, etc. No `Controller_NN` placeholders
- **Licensing:** GPL v2 (`Firmware/LICENSE`) due to DCS-BIOS dependency

# PCB Architecture

Each physical PCB is its own KiCad project. Controller groups live under a shared parent folder:

```
PCB/<Console>/<ControllerGroup>/
├── <ControllerGroup>_MCU/    ← main board (MCU + panel switches/LEDs merged)
├── <Panel_A>/                ← breakout board, harness to MCU board
└── <Panel_B>/                ← breakout board, harness to MCU board
```

Main boards have LEDs on the front side and all other components on the back side.
