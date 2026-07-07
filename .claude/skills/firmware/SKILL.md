---
name: firmware
description: OpenSkyhawk firmware — STM32 (PanelBridge, PanelGroup) and RP2040 (SimGateway). Use when writing or changing firmware, PlatformIO projects, CAN protocol, NODE_ID assignment, DCS-BIOS integration, HID, panel sketches, shared libraries or their test sketches, or interacting with MCP23017/ADS1115 through the firmware classes (PanelGroup input/output classes). PanelGroup work is the most common interaction.
---

# Firmware

Three-tier architecture:

```
PC ──USB HID + USB CDC(250k)── RP2040 SimGateway ──UART 250k── STM32 PanelBridge (NODE 0, CAN master)
                                                                      └─CAN 500k─ PanelGroup nodes (NODE 1–63)
```

## Authoritative spec — route here, don't bulk-load

`Firmware/ScratchPad/FirmwarePlan/` is the **authoritative** firmware specification (the *what*:
contracts, data flows, decisions) and supersedes any firmware content in `docs/_source/`.
`Firmware/ScratchPad/TechSpec/` holds the *how* (public API, class structure, method signatures).
Open the one relevant doc via the maps below rather than reading everything.

**FirmwarePlan document map** (read `FirmwarePlan/README.md` for the full index):

- `00-decisions.md` — why each choice was made
- `01-system-overview.md` — topology, library roles, data-flow narrative
- `02-can-protocol.md` — CAN frame IDs, `NODE_ID`, `ControlPacket` wire format, TX queue
- `03-uart-usb-hid-protocol.md` — UART mux, HID frame format, USB identity
- `04-dcs-bios-integration.md` — `controlId` space, `DCSIN_*`, input-map generator rules
- `05-panelgroup-api.md` — `PinRef`, MCP23017 management, **all input & output classes**
- `06-panelbridge-api.md` — node tracking, export listener, input dispatch, SYNC_REQ
- `07-simgateway-api.md` — `HIDAxis`/`HIDButton`, relay contract, HID dispatch
- `08-hardware-firmware-contracts.md` — STM32 pin assignments, DRV8833 ~SLEEP, ADS1115
- `09-startup-resync-diagnostics.md` — boot sequences, SYNC_REQ/READY, DIAG frames
- `10-implementation-plan.md` / `11-open-issues.md` — phased plan / deferred items

**TechSpec:** one file per library/class (`CANProtocol`, `STM32Board`, `PanelBridge`,
`SimGateway`, `HIDControls`, `A4ECGenerator`, and `PanelGroup/{PinRef, PanelGroup,
Inputs/*, Outputs/*}`). Each cites its FirmwarePlan section. See `TechSpec/README.md`.

**Availability guard:** these specs are committed but may be absent in a partial checkout —
if `Firmware/ScratchPad/FirmwarePlan/` is missing, rely on the conventions below plus the
committed code under `Firmware/Libraries/`.

## Durable conventions (always apply)

- **MCU variant:** `STM32F103C8` (64 KB) is the default for CAN avionics nodes; use
  `STM32F103CB` (128 KB) only where flash demands it (PanelBridge runs the full DCS-BIOS input
  map). Use full part numbers in prose, never the `CBT6` ordering code or `C8`/`CB` shorthand.
- **NODE_ID:** compile-time only — `build_flags = -DNODE_ID=N` in `platformio.ini`, never a
  `#define` in `main.cpp`. 0 = PanelBridge; 1–63 = PanelGroup nodes. Claim the next free
  number in `Firmware/NODE_IDS.md` in the same PR that starts the work; never reuse one.
  `tools/check_node_ids.py` enforces this in CI.
- **Expander access is via classes, not registers:** the `PanelGroup` library owns MCP23017
  integration; ADS1115 via `ADS1115.h`. Instantiate input classes (`Switch2Pos`, `Switch3Pos`,
  `RotarySwitch`, `SwitchMultiPos`, `AnalogInput`, `RotaryEncoder`, …) and output classes
  (`LED`, `IntegerOutput`, `SwitecX25Output`, …) on a `PinRef`. The *electrical* wiring of
  those parts belongs to the `pcb-design` skill.
- **Timing:** each class calls `millis()` directly and keeps its own `uint32_t` timestamp —
  no shared clock.
- **Docblocks:** Doxygen style on every public class/method/enum/non-obvious constant
  (`@brief`, `@param` with units, `@return`, `@note` for ISR/timing constraints). Private
  members use plain `//`.
- **Library + test layout:** each library is `Firmware/Libraries/<Name>/` with a `library.json`;
  its tests are a standalone project `Firmware/Tests/<Name>/` using `src_dir = tests`, an
  `[env_base]` section, and one `build_src_filter` per scenario. See `TechSpec/README.md`.
- **Implementation order (dependencies flow down):** HIDControls → A4EC → CANProtocol →
  STM32Board → PanelBridge / SimGateway → PanelGroup (PinRef → PanelGroup → Inputs/Outputs).
- **Header ownership — keep shared headers single-purpose.** `HIDControls.h` is **HID controlId
  constants only**; `CANProtocol.h` owns CAN frame transport (incl. `NodeHealthPayload`). The
  cross-node **status** contract — `NODE_STATUS_*` (DCS-BIOS reporting), `NodeHealthFlag`,
  `NodeFaultCode`, and `FaultSource` — lives in the neutral **`NodeStatus`** library, because every
  node type shares it (PanelGroup, PanelBridge, PDU) and it's broader than "health." Never shape a
  cross-node contract around one flavor (`PanelGroup.h`/`OutputBase`) or the HID namespace (D14).
- **Fault sources feed a node aggregator; no producer owns node health.** A fault-producing object
  (`DrumDisplay`, a PDU rail monitor, a PanelBridge host-link watchdog) **inherits
  `OpenSkyhawk::FaultSource`** (`NodeStatus.h`), self-registers, and reports *cached* `faultCode()`
  (a `NodeFaultCode`) + local `faultDetail()` (DiagSerial-only string, never on the wire). The
  node-level aggregator walks `FaultSource::head()` → sets `HEALTH_n.faultId` + DEGRADED. `OutputBase`
  gets **no** fault API — an output is just one possible fault source, not special.
- **Includes: direct for what you use, but don't couple upward.** Prefer a direct `#include` of a
  symbol's defining header over relying on a fragile transitive path — **but** if adding that
  include would drag a low-level/UI class into a heavier or conceptually-wrong layer, that's a
  signal the symbol is placed wrong (see the two rules above), not that you should hide it behind a
  transitive include. Don't add an include already reliably provided by a header the file *must*
  include anyway.
- **Spec-sync in the same PR.** Any change to a public firmware API — struct/enum, class method,
  wire format, `#define` contract — updates the authoritative `FirmwarePlan/` + `TechSpec/` doc in
  the **same PR**, listed as a deliverable and self-checked before push. The spec is source of
  truth; drift defeats it. Grep the old value/claim before pushing.
- **Toolchain:** PlatformIO. STM32 = `platform = ststm32`, `framework = arduino`. RP2040 =
  `earlephilhower` core with `-DUSE_TINYUSB`.
- **C++ standard:** any project using `DrumDisplay` needs `-std=gnu++20` +
  `build_unflags = -std=gnu++17` — its `DrumReadout` descriptors are C++20 designated
  initializers. Already baked into the `PanelGroup` scaffold template, so B6 sketches get it by
  default; add it explicitly to a standalone test project. The whole PanelGroup dep stack
  (U8g2 / MCP23017 / ADS1X15) is gnu++20-clean.

## Verify

Build the affected project(s) with `platformio run -d <project>`; exercise each class in
isolation via its `Firmware/Tests/<Name>/` sketch before integration.
