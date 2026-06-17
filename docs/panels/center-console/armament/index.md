---
status: new
---

# Armament Group

The Armament Group is OpenSkyhawk's **reference implementation** — the only panel group with
real hardware designed. In the A-4E it handles weapons management: the master arm system, the
bomb/rocket release system, the gunpod controls, the gunsight, and chaff dispensing.

It runs on a single PanelGroup controller at **NODE_ID 1**, with a host MCU board plus
breakout boards wired over I²C.

## Hardware status

| Panel | PCB Status | Firmware Status |
|-------|------------|-----------------|
| Armament Panel (MCU host) | Schematic complete | Phase 6 stub |
| AWRS Panel | ERC clean (ADS1115 @ 0x48); MODE SEL wiper → A2 not yet routed | Phase 6 stub |
| Misc Switch Panel | ERC clean, DRC unverified (MCP23017 @ 0x22) | Phase 6 stub |
| Gunpod Panel | Planned | Planned |
| Gunsight Panel | Planned | Planned |
| Chaff Panel | Planned | Planned |

Individual panel pages will be added as each panel is designed and built — there's no point
in an empty page per panel before the hardware exists.

!!! note "End-to-end testing in progress"
    The firmware is a Phase 6 stub: the panel group isn't yet integrated end-to-end through
    PanelBridge and SimGateway into DCS. This page will be expanded with bring-up and
    validation notes once that testing completes.

## Want to help?

The Armament Group is where the build is most real — schematics exist, the hardware is being
brought up. If you want to contribute to firmware integration, PCB review, or a not-yet-started
panel (gunpod, gunsight, chaff), start with [How to Contribute](../../../contributing/index.md).
