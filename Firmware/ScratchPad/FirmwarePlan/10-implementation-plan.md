# 10 — Implementation Plan

**Owns:** phased task list, migration notes, testing guidance.
**Does not own:** any spec content — this file only tracks what to build and when.

---

## Phase 0 — PlatformIO Project Templates *(prerequisite for everything)*

- [x] Create a PlatformIO project template for PanelBridge with `DCSBIOS_DEFAULT_SERIAL`
      define, `DcsBios::setup()` / `DcsBios::loop()` calls, and `PanelBridge::setup()` /
      `PanelBridge::loop()` calls. Template at `Firmware/Templates/PanelBridge/`.
- [x] Create a PlatformIO project template for SimGateway (RP2040) with TinyUSB joystick,
      USB CDC relay, UART link to PanelBridge, and empty HIDAxis/HIDButton declaration section.
      Template at `Firmware/Templates/SimGateway/`.
- [x] Create a PlatformIO project template for STM32F103C8 PanelGroup boards with:
  - `platformio.ini` pre-configured: correct board, framework (arduino), STM32duino platform,
    upload protocol (stlink), and `build_flags = -DNODE_ID=x` placeholder for the board's
    static node ID
  - Template comments state that `NODE_ID` must be set in `platformio.ini`; do not define
    `NODE_ID` in `src/main.cpp`, because library translation units also need the value
  - Correct clock source flags for external 8MHz crystal (required for reliable CAN timing)
  - `src/main.cpp` boilerplate: `#include <PanelGroup.h>`, empty wiring map section,
    empty outputs/inputs sections, `setup()` and `loop()` calling `PanelGroup::setup()` /
    `PanelGroup::loop()`
  - `STM32Board::setDebug(true)` in `setup()` with a comment to remove in production
  - Template at `Firmware/Templates/PanelGroup/`.
- [x] Verify templates compile clean against their target toolchains before any library code
      is written.

---

## Phase 1 — Core Shared Contracts *(prerequisite for bridge/gateway)*

- [x] Extract `HIDControls.h` from `CANProtocol` into its own standalone library at
      `Firmware/Libraries/HIDControls/` with `library.json` `"platforms": "*"`. This makes
      `CTRL_*` constants available to both STM32 (`CANProtocol`) and RP2040 (`SimGateway`)
      without pulling in the STM32-only `CANProtocol` library. Update `CANProtocol.h` to
      `#include <HIDControls.h>` instead of embedding the header directly.
- [x] Implement/update `STM32Board` per TechSpec: status LED, DiagSerial, CAN HAL init,
      `NODE_ID` validation, and `onCanStatus()` LED mapping.
- [x] Update `CANProtocol.h`: replace fixed `CAN_ID_HB_1`/`CAN_ID_EVT_1` etc. with
      `canIdHb(n)` / `canIdEvt(n)` / `canIdEcho(n)` / `canIdReady(n)` functions; add
      `SYNC_REQ` (0x012) frame constant; add a helper for filling `HeartbeatPayload` so
      PanelBridge and PanelGroup do not read HAL CAN registers directly.
- [x] Implement CANProtocol-owned `ControlPacketPair` batching for `CTRL_BCAST` and `EVT_n`
      frames. PanelBridge and PanelGroup submit individual `ControlPacket`s via
      `sendBatched()`. Slot B uses `controlId = 0x0000` when only one packet is present.
      Flush half-full batches no later than two owning firmware `loop()` iterations after
      slot A is queued; implementations may flush sooner. Do not apply pairing to
      special/control/diagnostic frames unless that frame type explicitly defines a
      `ControlPacketPair` payload.
- [x] Implement SW CAN TX queue policy in `CANProtocol`: when all 3 STM32 CAN TX mailboxes
      are occupied, enqueue frames in a small ring buffer (~16 entries). Use frame-type
      dependent overflow: `CTRL_BCAST` may drop/coalesce stale state; `EVT_n`, `READY_n`,
      `SYNC_REQ`, `TEST_SEQ`, and `ECHO_n` use bounded retry up to 3 attempts, then drop and
      increment the `DIAG_ERR` TX drop counter; `HB_n` may drop stale heartbeat frames.
- [x] Build A-4E-C input map generator: parse A-4E-C DCS-BIOS JSON → assign sequential
      `DCSIN_*` IDs from 0x8001; emit **three headers plus gap report**:
      - `A4EC_CmdIds.h` — `#define DCSIN_*` constants, included by PanelGroup sketches
      - `A4EC_OutputIds.h` — `#define A_4E_C_*` / `_AM` output address/mask constants
      - `A4EC_InputMap.h` — sorted `DcsBiosInputEntry[]` table keyed by `cmdId`, included by
        PanelBridge only
      - `GENERATOR_GAPS.md` — every skipped control with a reason
      Generator lives at `tools/gen_a4ec/gen_a4ec.py`. 150 inputs mapped (A4EC_CmdIds.h);
      0 gaps (all controls mapped — see GENERATOR_GAPS.md). 626 output IDs (A4EC_OutputIds.h).
      35 pytest tests passing. See `tools/gen_a4ec/README.md`.
- [x] **Migration note:** existing `CAN_ID_HB_1 = 0x100` changes to `canIdHb(1) = 0x101`.
      Deprecated aliases removed from `CANProtocol.h`; PanelGroup/PanelBridge fully migrated.

---

## Phase 2 — PanelBridge and SimGateway Backbone

### PanelBridge

- [x] Integrate DCS-BIOS library: configure `DCSBIOS_DEFAULT_SERIAL` to use `Serial`
      (UART2 PA2/PA3).
- [x] Add `ExportStreamListener` subclass that auto-registers on `PanelBridge::setup()` and
      submits all 0x8000–0x86FF writes to CANProtocol's `CTRL_BCAST` batching path.
- [x] Remove the UART argument from `PanelBridge::setup()` — DCS-BIOS library owns `Serial`
      directly via `DCSBIOS_DEFAULT_SERIAL`. Update `PanelBridge.h` to match.
- [x] Replace `MAX_NODES=2` with dynamic node tracking; hardware CAN filter remains pass-all;
      add software range validation in CAN RX handler (valid child-node frames: HB_1-HB_63
      0x101–0x13F, EVT 0x201–0x23F, ECHO 0x301–0x33F, READY 0x401–0x43F — discard else).
- [x] Broadcast `SYNC_REQ` on READY and when heartbeat tracking observes a node transition
      from dead/unseen to alive. PanelBridge does not publish `HB_0`; `canIdHb(0)` is reserved.
- [x] Implement TEST_SEQ trigger on PanelBridge: a single command byte (e.g. `'T'`) received
      on DiagSerial causes PanelBridge to broadcast TEST_SEQ (0x011) and collect ECHO responses
      for RTT measurement. SimGateway is not involved.
- [x] Implement input map dispatch inside `PanelBridge::loop()`:
      `0x8000 <= controlId <= 0x86FF` → binary search → `sendDcsBiosMessage()`;
      `controlId < 0x8000` → wrap in HID frame → UART to SimGateway.
- [x] Add PanelBridge tests for DCS routing boundaries (`0x86FF` routes, `0x8700` and
      `0xFFFF` are dropped) and model-time resync (first value seeds state; later decrease
      broadcasts `SYNC_REQ`).

### SimGateway

- [x] Refactor `SimGateway::loop()` to relay raw bytes USB CDC ↔ UART; no DCS-BIOS library calls.
- [x] Implement HID frame demultiplexer: watch for `HID_MAGIC` on UART RX; forward all other
      bytes to USB CDC.
- [x] Implement `HIDAxis` and `HIDButton` self-registering classes.
- [x] Replace raw `_cbEvt` callback with HID linked-list dispatch: `controlId < 0x8000` →
      walk `HIDAxis`/`HIDButton` list → HID report setters; send one report once after
      draining if any setter fired.
- [x] Add SimGateway on-device harness tests that inject synthetic HID-frame bytes through
      `SIMGATEWAY_TEST` parser hooks and, optionally, `Serial1` TX→RX loopback. These tests
      validate parser resync, HID dispatch, value mapping, and send batching. Do not test local
      ADC, I2C ADCs, sensors, CAN, or PanelGroup hardware in SimGateway.

---

## Phase 3 — Basic PanelGroup Ecosystem *(DONE — merged PR #26)*

Goal: prove the smallest useful chain with `SimGateway`, `PanelBridge`, and one `PanelGroup`
node that has one switch input and one LED output mapped through the generated IDs. Only
items marked **Ready for implementation** in TechSpec are in scope for this phase.

- [x] Design and implement `PinRef` class (GPIO, MCP23017, ADS1115 variants).
- [x] Implement PanelGroup core: registration lists, `setup()`, `loop()`, batched EVT send,
      `CTRL_BCAST` dispatch, `SYNC_REQ` response, heartbeat, READY frame, and
      CANProtocol-owned `TEST_SEQ` auto-reply via `drain()`.
- [x] Update `PanelGroup`: use `NODE_ID` (injected via `build_flags = -DNODE_ID=x` in
      `platformio.ini`) for all CAN IDs — do not use `#define NODE_ID` in `main.cpp`.
- [x] Implement MCP23017 management in PanelGroup: `registerExpander()`, interrupt setup,
      INTCAP dispatch (interrupt-driven + 20 ms polling fallback for NO_INT_PIN chips).
- [x] Implement `LED` using `PinRef` and `OutputBase::onControlPacket()`.
- [x] Update `Switch2Pos` to accept `PinRef` instead of `uint8_t pin`.
- [x] End-to-end integration sketch (one `Switch2Pos` + one `LED` through PanelBridge and
      SimGateway) — done as `Firmware/Examples/E2E_DCS_Test`, hardware-verified against live DCS
      2026-06-15 (PR #30); it has since grown into the one-instance-of-every-class integration node.

**Hardware verification (STM32F103CBT6 Blue Pill clone, 2026-06-12):**

| Test env | Hardware | Result |
|---|---|---|
| test_nc_sentinel | STM32 only | PASS |
| test_gpio_identity | STM32 only | PASS |
| test_gpio_digital | STM32, PB0→PA0 + PB1→PA1 jumpers | PASS |
| test_gpio_analog | STM32, PA1 at 1.65 V mid-rail | PASS |
| test_ads1115_analog | STM32 + ADS1115 @ 0x48 (I2C1 remap PB8/9), A0 at 1.65 V | PASS |
| test_boot_sequence | STM32, CAN loopback | PASS |
| test_heartbeat | STM32, CAN loopback | PASS |
| test_ctrl_bcast | STM32, CAN loopback | PASS |
| test_sync_req | STM32, CAN loopback | PASS |
| test_can_drain | STM32, CAN loopback | PASS |
| test_register_adc | STM32 + ADS1115 @ 0x48 (I2C1 remap PB8/9), A0 at 1.65 V | PASS |
| test_interrupt_dispatch | STM32 + MCP23017 @ 0x20 (I2C1 remap PB8/9), switch on GPA0 | PASS |

**Key implementation decisions recorded during Phase 3:**
- ADS1115 gain: `GAIN_ONE` (±4.096V FSR) set in PinRef constructor — 3.3V → ~52800/65534.
- GPIO ADC resolution: `STM32Board::begin()` calls `analogReadResolution(16)`; framework
  scales 12-bit → 0–65520 internally. PinRef does no shifting.
- MCP23017 GPA7/GPB7 silicon erratum: GPINTEN masked to 0x7F on both ports during setup.
- MAX_INT_PINS (8 unique STM32 interrupt pins) overflow: logs warning, falls back to polling.
- `_rxCount` (heartbeat rx counter) is `uint16_t`; intentional rollover at ~131 s.

> **Testing note:** narrow input class unit tests may still use hand-written placeholder
> constants (e.g. `#define DCSIN_TEST_SW 0x8001`). Integration tests should use the generated
> `A4EC_CmdIds.h` from Phase 1.

---

## Phase 4 — Remaining Input Types *(core pass DONE — the A-4E-C's inputs are all covered)*

Checked 2026-09-21 against the DCS-BIOS `A-4E-C.jsonp` export (150 inputs): every input control has
a class. Push-to-set knobs (`IAS_INDEX`, `STBY_ATT_INDEX`, `RADAR_ALT`) are a `Switch2Pos` plus a
`RotaryEncoder` — DCS-BIOS exposes separate button and knob identifiers. Class structure follows
D16 (families, DCS-BIOS names).

- [x] `Switch3Pos` — hardware-verified 2026-06-23 (PR #144, #115).
- [x] `SwitchMultiPos` + the `MultiPosInput` base — hardware-verified 2026-06-22 (PR #140, #124).
- [x] `AnalogMultiPos` on `MultiPosInput` — hardware-verified 2026-06-23 (PR #143, #114).
- [x] `AnalogInput` — hardware-verified 2026-08-14 on PanelGroup Rev 1 (PR #145, #118); per-instance
      `pollMs` (PR #267); HID-axis calibration lives in SimGateway (#251).
- [x] `RotaryEncoder` with REL / DIR modes over dedicated CAN frames — live-DCS verified (PR #146 / #117,
      PR #157 / #147).
- [x] `ActionButton` — one TOGGLE per press on `EVT_ACTION_n` (PR #275, #116). Hardware + live-DCS
      check happens in the v1.0 soak panel smoke test (#291).
- [x] `RotaryAcceleratedEncoder` — thin `RotaryEncoder` subclass, DCS-BIOS parity: momentum filter
      + `fastStep` (#287). Hardware-verified 2026-09-21 (5/5 envs; the 8 RotaryEncoder envs re-run 8/8).
- [ ] `AngleSensorInput` : `AnalogInput`, on a `PinRef` (the sensor's analog output) — for
      absolute knobs such as `GUNSIGHT_KNB` (#294). **Firmware v0.1.0**, so the class set is locked
      before v1.0. Spec: `TechSpec/PanelGroup/Inputs/AngleSensorInput.md`.
- [ ] `SwitchWithCover2Pos` — DCS-BIOS behaviour (one pin; sequences the cover) on a protected
      `Switch2Pos` hook (#293). **Firmware v0.1.0.** The A-4E-C itself uses it 0× — its only guard,
      AFCS 1-N-2, is a 3-position switch the mod doesn't gate, so it is a `Switch3Pos` (+ an
      optional `Switch2Pos` on a cover microswitch); the class is for DCS-BIOS parity.
- ~~`RotarySwitch`~~ — **dropped** (D16): `RotaryEncoder` DIR drives bounded selectors without the
      boot-position problem.

---

## Phase 5 — Output Types

Checked against the same export (152 outputs): LEDs, needles/flags/ADI and drum digits are
covered; the only uncovered outputs were the five light-intensity values (`LIGHTS_CONSOLE`,
`LIGHTS_INSTRUMENTS`, `LIGHTS_FLOOD_RED`, `LIGHTS_FLOOD_WHITE`, `APG53A_GLOW`).

- [x] `LED`.
- [x] `NeedleGauge` + the `MotorDriver` / `StepperMotor` layer — supersedes the former
      `SwitecX25Output` / `AccelStepperOutput` / `ServoOutput` (one gauge class over a swappable
      backend; STALL or sensor homing, non-blocking update). (#122 / #131)
- [x] `DrumDisplay` — OLED rolling-drum readouts behind `I2cMux` (PR #129, #113); leading-zero
      suppression (PR #204, verification #196).
- [ ] `AnalogOutput` family — abstract base + `Dimmer` (PWM backlight zones, the five
      light-intensity outputs) + `IntegerOutput` (user callback) (#288). **Firmware v0.1.0.**
- [ ] `ServoMotor` `MotorDriver` backend (servo-driven pointers). (#132) — after v1.0.

---

## Phase 5b — Infrastructure Nodes: PDU *(Firmware v0.2.0)*

- [ ] PDU boards (#202) assembled and brought up.
- [ ] PDU monitoring firmware + its new CAN telemetry frame (#216) — reuse the `HEALTH_n` pattern
      (D13).

---

## v1.0 Release Gate *(Firmware v1.0.0)*

v1.0.0 freezes the sketch API and the CAN wire format — after it, a breaking change means 2.0 — and
ships **before** Phase 6 panel work. Anything that changes an API or the wire format lands before
it; anything that doesn't (#269, #132, #228) can land after without breaking anyone. Every control
class is locked in v0.1.0. The gate itself is tracked in #290 and runs on the hardware from the
"Base boards Rev 2 + soak test rig" milestone (#209, #95, #291):

- [ ] Multi-node live-DCS soak, 2–3 h: no false offline, `DIAG_ERR` TX-drop counters at zero,
      reconnect / `SYNC_REQ` keeps mission presets.
- [ ] #291 smoke test passed — the single live check for every in-scope class.
- [ ] CAN verified at 500 kbps against the scope (#209's pre-order checks).
- [ ] API + wire review: constructor signatures, CAN ID map, payload layouts.

---

## Phase 6 — First Full Panel Integration *(after v1.0)*

The first controller is **Right_Navigation** (#168: ASN-41 host + APN-153 and ARC-51A I²C
sub-panels). Center_Armament was the original target, but its MCU board is retired; its
sub-panels get redesigned to the harness standard when that controller is picked up.

- [ ] Right_Navigation PanelGroup sketch using the full library (#172, B6).
- [ ] Add HID declarations to the SimGateway sketch for the stick sub-node axes.
- [ ] End-to-end test: DCS running → panel control → DCS-BIOS command via PanelBridge, and DCS
      outputs → panel gauges / drums / lights (#173, B9).
