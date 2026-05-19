```
   ___                    ____  _          _   _               _    
  / _ \ _ __  ___ _ __   / ___|| | ___   | | | | __ ___      | | __
 | | | | '_ \/ _ \ '_ \  \___ \| |/ / | | | |_| |/ _` \ \ /\ / / |/ /
 | |_| | |_) |  __/ | | |  ___) |   <| |_| |  _  | (_| |\ V  V /|   < 
  \___/| .__/ \___|_| |_| |____/|_|\_\\__, |_| |_|\__,_| \_/\_/ |_|\_\
       |_|                            |___/                            

              ── DCS A-4E Skyhawk Home Cockpit ──
```

Physical DCS A-4E Skyhawk home cockpit — 3D-printed panels, custom PCBs, and STM32 firmware for the [DCS A-4E Community Mod](https://github.com/Community-A-4E/community-a4e-c). Controllers communicate over CAN bus.

## Repository Structure

```
OpenSkyhawk/
├── CAD/                        # Fusion 360 source files (.f3d); STLs gitignored
│   ├── Center_Console/
│   ├── Left_Console/
│   ├── Right_Console/
│   └── Shared/                 # Common brackets, mounting fixtures
│
├── PCB/                        # KiCad projects, one folder per board
│   ├── Center_Console/
│   │   └── Center_Armament/
│   │       ├── Armament_MCU/   # Main STM32 board
│   │       ├── AWRS_Panel/     # Breakout board
│   │       └── Misc_Switch_Panel/
│   ├── Left_Console/
│   ├── Right_Console/
│   └── Libraries/              # Shared KiCad symbols and footprints
│
├── Firmware/                   # PlatformIO projects, one folder per STM32 controller
│   ├── Center_Armament/
│   └── Libraries/              # Shared firmware code
│
└── Docs/
    ├── References/             # Cockpit photos, manuals, screenshots
    ├── Datasheets/             # Component datasheets
    └── claude/                 # AI-readable architecture and standards docs
```

## Architecture

Each physical panel group is driven by one **STM32F103CBT6** MCU board. Boards communicate over **CAN bus** via Molex Mini-Fit Jr connectors. Panel breakout boards connect to their MCU board via 6- or 8-pin JST-XH harnesses. Firmware is built with **PlatformIO**.

DCS communication: DCS-BIOS cockpit state flows PC → USB CDC → RP2040 bridge (Tiny2040) → UART → STM32 CAN master → CAN bus → all avionics nodes. STM32 native USB CDC is not used — it crashes under sustained DCS-BIOS load.

## Console Layout

| Console | Panels |
|---------|--------|
| Center Console | Armament, AWRS, Misc Switches, main flight instruments |
| Left Console | ECM, fuel, electrical systems |
| Right Console | Radios, nav |

## Hardware Notes

### Screw Standards

| Size | Use |
|------|-----|
| M2 | PCB mounts, small standoffs |
| M3 | Placards, light rings, small brackets |
| M4 | Instrument bezels, gauge mounting — clearance Ø4.3–4.5 mm |
| M5 | Panel-to-subpanel, corner mounts — clearance Ø5.3–5.5 mm |

### Switch Sizing

- Toggle switches: 12 mm (standard panels), ~6 mm (ECM modules)

### Gauges

- LOX gauge: 2-5/8″ (~67 mm total)
- Radar Altimeter: 3-1/8″ (~100 mm total with bezel)

## Licensing

| Layer | License |
|-------|---------|
| CAD, PCB, Docs | CC BY-NC-SA 4.0 |
| Firmware | GPL v2 (DCS-BIOS dependency) |

## References

- [Open Hornet Hardware](https://github.com/jrsteensen/OpenHornet)
- [Open Hornet Software](https://github.com/jrsteensen/OpenHornet-Software)
- [The Warthog Project](https://thewarthogproject.com/)
- [Viperpits](https://www.viperpits.org/smf/index.php)
- [The Skyhawk Association — Cockpit](https://skyhawk.org/page/skyhawk-cockpit)
