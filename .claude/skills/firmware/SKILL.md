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
- `09-startup-resync-diagnostics.md` — boot sequences, SYNC_REQ/READY, DiagSerial diagnostics
- `10-implementation-plan.md` / `11-open-issues.md` — phased plan / deferred items

**TechSpec:** one file per library/class (`CANProtocol`, `STM32Board`, `PanelBridge`,
`SimGateway`, `HIDControls`, `A4ECGenerator`, `PowerMonitor` (the PDU node), and `PanelGroup/{PinRef, PanelGroup,
Inputs/*, Outputs/*}`). Each cites its FirmwarePlan section. See `TechSpec/README.md`.

**Availability guard:** these specs are committed but may be absent in a partial checkout —
if `Firmware/ScratchPad/FirmwarePlan/` is missing, rely on the conventions below plus the
committed code under `Firmware/Libraries/`.

## Durable conventions (always apply)

- **MCU variant:** `STM32F103C8` (64 KB) is the default for **every** CAN board, PanelBridge
  included (it compiles to ~26 KB; `board = genericSTM32F103C8` in every template and panel).
  The `STM32F103CB` (128 KB) is a drop-in fallback on the same footprint, used only when a
  specific build outgrows 64 KB, and no board needs it today. Before moving a build to the CB,
  try link-time optimisation (`-flto`, as the bootloader uses): it saves a lot of flash. Bench-check
  ISR timing (ShiftBus) after turning it on. Use full part numbers in prose, never the `C8T6`/`CBT6`
  ordering codes or the `C8`/`CB` shorthand.
- **NODE_ID:** compile-time only — `build_flags = -DNODE_ID=N` in `platformio.ini`, never a
  `#define` in `main.cpp`. 0 = PanelBridge; 1–63 = PanelGroup nodes. Claim the next free
  number in `Firmware/NODE_IDS.md` in the same PR that starts the work; never reuse one.
  `tools/check_node_ids.py` enforces this in CI.
- **Expander access is via classes, not registers:** the `PanelGroup` library owns MCP23017
  integration; ADS1115 via `ADS1115.h`. Instantiate input classes (`Switch2Pos`, `Switch3Pos`,
  `SwitchMultiPos`, `AnalogInput`, `RotaryEncoder`, `ActionButton`, …) and output classes
  (`LED`, `NeedleGauge`, `DrumDisplay`, the `AnalogOutput` family, …) on a `PinRef`. Classes come
  in **families** with DCS-BIOS names (FirmwarePlan D16): a new class that only changes an existing
  one's source or sink is a subclass of it. The *electrical* wiring of those parts belongs to the
  `pcb-design` skill.
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
  truth; drift defeats it. Before pushing, grep for the old value **and the old behaviour's
  wording** (e.g. "coalesc", "broadcast", "once per boot", a renamed class or listener) across
  `FirmwarePlan/`, `TechSpec/`, `docs/`, library READMEs and header docblocks. A value grep alone
  missed most of the drift in the 2026-10-27 audit. Update **every** TechSpec that describes the
  behaviour, not only the one whose API changed. When the change contradicts a decision in
  `00-decisions.md`, amend or supersede **every** entry it overrides (#392 left D7/D24 describing
  removed coalescing, and #390 left D5 describing a broadcast `SYNC_REQ`).
- **Published docs sync too, when the change is visible from them.** A new control class, a new
  CAN frame family, or a change to what an existing frame carries also updates `docs/firmware/`
  and `docs/architecture/` in the **same PR** — at feature/overview altitude, not a second copy
  of the TechSpec. These pages go stale fastest:
  - `docs/firmware/index.md` (what is built);
  - `docs/firmware/control-types.md` (per-class status);
  - `docs/architecture/can-bus.md` (the frame table and priorities);
  - `docs/firmware/compatibility.md` (every reserved export address, PC message and contract
    between the Gateway's two chips);
  - `docs/guides/bring-up.md` (boot and sync behaviour);
  - `docs/guides/new-control-type.md` (the `InputBase`/`OutputBase` contract);
  - `docs/firmware/build-flags.md` (any new `-D` override);
  - FirmwarePlan `09` (any added or reordered boot step).

  The weekly docs-drift review is a backstop for what slips through, not the mechanism: in #284
  four of five findings had been wrong since #124, #147 and #222, because each feature PR left
  them behind.
- **Closing a ticket.** Before a PR closes a ticket, closing secondary ones included, tick each
  Done-when box with where its evidence is ("— #PR", a bench table), or move the unmet item to a
  named follow-up and say so on the ticket. When a bench check was swapped for an equivalent, or
  not re-run after the code path changed, say that too. In the 2026-10-27 audit every closed
  firmware ticket had unticked boxes and unrecorded or substituted checks.
- **Release facts come from the tag.** Never write "in firmware vX" from a milestone or from
  timing. Check it: `git merge-base --is-ancestor <commit> firmware-vX`. `firmware-v0.2.0` was cut
  7 minutes before #359, and client tickets built on the wrong guess. Releases before 1.0.0 are
  test-only; 1.0.0 is the first public one.
- **No new reporting without a need.** Don't add frames, payload fields, flag bits or `DIAG`
  fields for visibility's sake, because every report costs bus or link time. Loss counters
  (`rxOverrunCount()`, `txDropCount()`, `rxDropCount()`) stay on the board, readable in debug or
  bench builds and through SoakRig. Docs never claim more than that: "never lost silently" means
  *counted on the board*, not reported.
- **Responsive inputs first.** An input (switch or knob → sim) must register promptly and never
  be lost. A sim → cockpit update that arrives late or drops once is acceptable, because DCS-BIOS
  resends every value. Judge bus and loop changes, and write pass criteria, on that basis: input
  loss or input latency fails; a recorded output drop that catches up doesn't (D31).
- **Toolchain:** PlatformIO. STM32 = `platform = ststm32@^20.0.0`, `framework = arduino`. RP2040 =
  `earlephilhower` core with `-DUSE_TINYUSB`.
- **C++ standard:** any project using `DrumDisplay` needs `-std=gnu++20` +
  `build_unflags = -std=gnu++17` — its `DrumReadout` descriptors are C++20 designated
  initializers. Already baked into the `PanelGroup` scaffold template, so B6 sketches get it by
  default; add it explicitly to a standalone test project. The whole PanelGroup dep stack
  (U8g2 / MCP23017 / ADS1X15) is gnu++20-clean.

## Verify

Build the affected project(s) with `platformio run -d <project>`; exercise each class in
isolation via its `Firmware/Tests/<Name>/` sketch before integration.
