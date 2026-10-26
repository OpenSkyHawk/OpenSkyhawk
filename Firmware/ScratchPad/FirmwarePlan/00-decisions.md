# 00 — Architecture Decisions

**Owns:** rationale for every non-obvious architectural choice made during design.
**Does not own:** the actual specs — those are in the contract files. This file explains *why*,
not *what*.

Each decision is a one-way door that other files depend on. If a decision changes, every
dependent file must be reviewed.

---

## D1 — SimGateway is a pure byte relay; PanelBridge owns DCS-BIOS

**Decision:** SimGateway forwards raw bytes between USB CDC and UART without parsing them.
PanelBridge runs the DCS-BIOS library.

**Rationale:** STM32 native USB CDC crashes under sustained DCS-BIOS data flow (socat exits
"Permission denied"; port becomes unusable until replug). The RP2040 USB stack handles
250 000 baud DCS-BIOS traffic without issue. Putting the DCS-BIOS library on the RP2040
would complicate SimGateway and split the processing across two devices that communicate
via a single UART — worse latency and harder to debug. PanelBridge on STM32 handles the
DCS-BIOS library cleanly because it never uses USB at runtime.

**Affects:** `01-system-overview.md`, `06-panelbridge-api.md`, `07-simgateway-api.md`.

---

## D2 — controlId routing: 0x8000-0x86FF → DCS-BIOS, < 0x8000 → HID

**Decision:** `controlId` values determine routing path without any additional lookup. Values
in `0x8000`-`0x86FF` are DCS-BIOS compact command IDs routed by PanelBridge; values
`< 0x8000` are HID axes/buttons routed by SimGateway. Values outside both valid routing
spaces, including `0xFFFF`, are reserved/invalid and must not be treated as DCS-BIOS input
commands.

**Rationale:** The A-4E-C mod has no axis exports (stick/rudder are HID-only inputs). The two
ranges therefore never overlap. The DCS-BIOS input range is still bounded so reserved IDs such
as `0xFFFF` cannot accidentally route into the generated input map.

**Affects:** `04-dcs-bios-integration.md`, `06-panelbridge-api.md`, `07-simgateway-api.md`.

---

## D3 — DCSIN_* as compact transport aliases (not device IDs or DCS output addresses)

**Decision:** `DCSIN_*` constants are compact stand-ins for DCS-BIOS command name strings,
required because a CAN frame carries only 8 data bytes — too little for an ASCII command like
`"LIGHT_EXT_MASTER 2\n"`. Multiple input controls may share one `DCSIN_*` constant if they
trigger the same DCS command. PanelBridge does not track which node sent the EVT.

**Rationale:** DCS output addresses identify values *from* DCS to the cockpit. Input controls
send commands *to* DCS. The namespaces are different. Reusing output addresses as input IDs
would conflate two independent concepts. The `DCSIN_*` approach decouples the physical control
from the DCS command it triggers and avoids the 8-byte CAN frame limitation.

**Affects:** `04-dcs-bios-integration.md`, `05-panelgroup-api.md`.

---

## D4 — 16-bit normalisation for all analog inputs (0–65535)

**Decision:** all analog input sources (STM32 ADC, ADS1115, AS5600, MT6701) are normalised to
16-bit before sending over CAN. The 16-bit value is used as-is at the destination — no
rescaling in PanelBridge or SimGateway.

**Rationale:** DCS-BIOS `set_state` controls with `max_value > 1` expect 0–65535 (or integer
strings thereof). Using one resolution throughout eliminates per-destination scaling code and
makes CAN traffic predictable.

The original rationale also cited `Joystick.use16bit()`, from a HID library evaluated before
TinyUSB was selected (D-HID / `03-uart-usb-hid-protocol.md#hid-backend`). That API no longer
exists in the codebase; the decision is unaffected, because 16-bit unsigned remained the right
choice for the reasons above. The backend change moved the conversion rather than the
convention: `HIDAxis::dispatch()` now applies calibration and then `value − 32768`.

The convention has since spread further — axis calibration stores and transmits unsigned
0–65535 for the same reason, and a client carries it unsigned through everything that stores,
compares or sends a value. The conversion stays where it was moved to: `HIDAxis::dispatch()`,
once, on the way into the HID report.

A client may still show the signed view, and a calibration UI should: the number on screen is one
the user will also read in DCS, which shows ±32767. That is a rendering choice at the display
edge, not a change to the convention — the value being committed is unsigned either way.

**Affects:** `05-panelgroup-api.md`, `07-simgateway-api.md`.

---

## D5 — SYNC_REQ / READY handshake for boot ordering and node recovery

**Decision:** PanelBridge broadcasts `SYNC_REQ` (0x012) on cold boot, on DCS session change,
when a PanelGroup sends `READY`, and when a node transitions from dead/unseen to alive. Each
PanelGroup sends a `READY` frame (`0x400+n`) after completing its initial input poll.
PanelGroup also re-polls on any subsequent `SYNC_REQ` it receives.

**Rationale:** PanelGroup nodes may boot before PanelBridge (power sequencing). Without a
handshake, the boot-time input EVT burst is lost and DCS starts with stale switch positions.
The SYNC_REQ mechanism ensures PanelBridge can request a fresh input snapshot at any time —
whether after READY, on cold boot, after a node reconnect, or after a DCS session change.

Missed outputs (LEDs, gauges) are tolerable — a fresh DCS-BIOS update arrives within ~50 ms.
Missed inputs (switch positions) would leave DCS in the wrong state until the next physical
change — this is not tolerable.

**Affects:** `02-can-protocol.md`, `05-panelgroup-api.md`, `06-panelbridge-api.md`,
`09-startup-resync-diagnostics.md`.

---

## D6 — HID frame magic bytes: 0xAA 0x55

**Decision:** HID frames use `0xAA 0x55` as the two-byte header (both bytes have bit 7 set).

**Rationale:** DCS-BIOS ASCII text commands use only printable ASCII + LF (all bytes ≤ 0x7F).
`0xAA` and `0x55` both have bit 7 set, making collision with DCS-BIOS bytes structurally
impossible. No parser state machine needs to be maintained for the common case — any byte
≤ 0x7F is forwarded immediately to USB CDC. *(Superseded in part by D20: the host-bound side now
forwards whole lines rather than single bytes.)*

**Affects:** `03-uart-usb-hid-protocol.md`.

---

## D7 — CAN TX queue overflow: frame-type dependent

**Decision:** CAN TX overflow policy depends on frame type:

| Frame type | Policy |
|------------|--------|
| `CTRL_BCAST` DCS output broadcasts | Drop stale queued state and retain the newest frame. Coalesce by `controlId` where practical. |
| `EVT_n` input events | Bounded retry: keep the event for up to 3 send attempts, then drop it and increment the `DIAG_ERR` TX drop counter. Never build an unbounded backlog. |
| `HB_n` heartbeat | Drop stale heartbeat; the next heartbeat replaces it. *(As built, heartbeats — and the later `HEALTH_n` / `TELEM_n` — take the same bounded retry as event frames; only `CTRL_BCAST` coalesces. See `02-can-protocol.md`.)* |
| `READY_n`, `SYNC_REQ`, `TEST_SEQ`, `ECHO_n` | Bounded retry up to 3 attempts, then drop and increment the `DIAG_ERR` TX drop counter. |

**Rationale:** output broadcasts are state snapshots; the newest value is always more useful
than stale intermediate gauge/LED values. Input events are discrete user actions and should
not use stale-output drop-oldest semantics, but they also must not create an unbounded backlog
when the bus is unhealthy. Three attempts is enough to ride out transient mailbox contention;
if drops occur in normal traffic, the diagnostic counter exposes a bus/load problem.

**Affects:** `02-can-protocol.md`, `06-panelbridge-api.md`.

---

## D8 — ANALOG_NC = 0xFFFF

**Decision:** `AnalogMultiPos` uses `0xFFFF` as the sentinel for positions with no physical
detent.

**Rationale:** STM32 ADC tops at 65520 after ×16 scaling; ADS1115 tops at 65534 (its 0–3.3V
span is stretched onto the 16-bit range and clamped one under `0xFFFF` — #325). `0xFFFF` (65535)
is unreachable on both hardware paths. No separate
boolean is needed in the position array.

**Affects:** `05-panelgroup-api.md`.

---

## D9 — RotaryAcceleratedEncoder 4-value scheme (not delta + direction separately)

> **Superseded by #147.** The encoder rework moved variable-step knobs to REL — a signed `±step` on
> its own frame (`canIdEvtRel`) — and collapsed the input map to `controlId → name`, removing the
> `arg0`/`arg1`/`arg0fast`/`arg1fast` fields this scheme dispatched through. Acceleration is now a
> larger `±step` on the same REL frame, with no wire, bridge or map change; the class is a thin
> `RotaryEncoder` subclass under D16. Kept below for history.

**Decision (historical):** `RotaryAcceleratedEncoder` encodes both direction and speed into a single uint16
value: 0=slow CCW, 1=slow CW, 2=fast CCW, 3=fast CW. PanelBridge maps these to four
`DcsBiosInputEntry` arg fields.

**Rationale:** each `ControlPacket` carries only one value. Encoding direction+speed into
2 bits is compact and requires no additional struct fields in `ControlPacket`. The 4-value
scheme mirrors the DCS-BIOS Arduino library's accelerated encoder model exactly, avoiding any
mismatch between firmware and DCS-BIOS argument strings.

**Affects:** `05-panelgroup-api.md`, `04-dcs-bios-integration.md`.

---

## D10 — NODE_ID via PlatformIO build_flags

**Decision:** `NODE_ID` is declared as `build_flags = -DNODE_ID=3` in `platformio.ini`, not
as `#define NODE_ID 3` in `main.cpp`.

**Rationale:** a `#define` in `main.cpp` is visible only in that translation unit. The CAN
protocol library and other TUs need `NODE_ID` to compute frame IDs. `build_flags` injects it
as a compiler flag visible to all TUs in the project.

**Affects:** `02-can-protocol.md`, `05-panelgroup-api.md`.

---

## D11 — CAN batching via ControlPacketPair

**Decision:** `CTRL_BCAST` and `EVT_n` frames carry a `ControlPacketPair` so two
ControlPackets can share one 8-byte CAN frame. If only one packet is ready, slot B uses
`controlId = 0x0000` as the null sentinel. CANProtocol owns the batch builders and TX queue
state; PanelBridge and PanelGroup submit individual `ControlPacket`s. A half-full batch may
wait for slot B for at most two owning firmware `loop()` iterations after slot A is queued,
then it must be flushed with a null slot B. Here "cycle" means main firmware loop iteration,
not an MCU clock cycle.
Input snapshot bursts, such as boot and `SYNC_REQ` re-polls, flush any odd trailing slot-A
packet immediately at the end of the poll pass.

Batching is only required for input/output routing frames (`EVT_n` and `CTRL_BCAST`). Special
frames use their own explicitly defined DLC and payload. A future/special frame may carry a
single 4-byte `ControlPacket` without pairing, but that frame type must say so explicitly.

**Rationale:** batching uses the full classic CAN payload and cuts frame count for DCS output
bursts and input snapshot bursts without changing the simple `ControlPacket` routing model.
The null sentinel keeps single-packet sends trivial. The short batching deadline matters most
for `EVT_n` input traffic: user input should not sit in a half-full batch waiting for another
event that may not arrive.

**Affects:** `02-can-protocol.md`, `06-panelbridge-api.md`, `09-startup-resync-diagnostics.md`.

---

## D12 — HB_0 is reserved but not transmitted

> **Superseded by #93 (2026-06-17).** PanelBridge now transmits `HB_0` every 500 ms as an
> unconditional master-liveness beacon, and every other node raises WARNING when it stops — see
> `08-hardware-firmware-contracts.md`. Since #340 the sending and the watchdog both live in
> `CANProtocol::serviceNodeStatus()`. The text below is kept as the original record.

**Decision:** `canIdHb(0)` (`0x100`) remains reserved by the CAN ID formula for PanelBridge,
but PanelBridge does not transmit a master heartbeat in normal firmware.

**Rationale:** PanelBridge is the CAN master and no production node consumes a master heartbeat.
Sending `HB_0` adds traffic and implementation surface without a receiver. PanelBridge health
is diagnosed through its own DiagSerial output and through the presence or absence of routed
CAN/DCS traffic.

**Affects:** `02-can-protocol.md`, `06-panelbridge-api.md`.

---

## D13 — Internal die-temp telemetry on a new HEALTH_n frame, uncalibrated, no default trip (#213)

**Decision:** every STM32 node reads its MCU internal temperature sensor (ADC ch16, using Vrefint
ch17 internally to reference the reading to Vdd) and reports `dieTempC` on a **new dedicated
`HEALTH_n` frame** (`0x140+n`, 1000 ms, default-on). It is **not** folded into `HB_n`. Vdd is
measured but **not transmitted** — per-node rail voltage is weak signal on a regulated bus and is
covered properly by the PDU's power telemetry (`TELEM_n`, #216; INA180A2 current sense, #202). The overheat flag / status-LED
WARNING is computed only when a build defines `NODE_OVERHEAT_C`; the default build ships pure
telemetry with no trip point. The frame is the shared node-health contract (#221): temperature
(#213) and degraded-state (#163) each own their own fields within the fixed 8 bytes.

**Rationale:**
- *New frame, not repack.* The 8-byte `HeartbeatPayload` is already full, and its consumers
  (PanelBridge tracking, the client's `_NODE_STATUS` parser) depend on its exact layout.
  A separate frame leaves that contract untouched and gives room for future health fields
  (3 reserved bytes). The `0x140–0x17F` range was free.
- *Free telemetry.* The sensor is on-die — no external parts, no schematic/BOM change — so the
  convention applies to every existing and future STM32 board unchanged.
- *No default trip.* The F103 sensor is uncalibrated (no factory trim) and reads die (not
  ambient) temperature with a self-heat offset, so a meaningful overheat threshold needs field
  data first. Shipping telemetry-only lets the fleet collect trend data before a trip point is
  chosen; enabling it is a one-line `-DNODE_OVERHEAT_C=N` opt-in.
- *STM32 only.* The RP2040 SimGateway is not on CAN and shares a PCB with PanelBridge, whose
  `HEALTH_0` already represents that board's thermal zone — a second sensor there is redundant.

**Affects:** `02-can-protocol.md`, `08-hardware-firmware-contracts.md`, `docs/_source/hardware-standards.md`,
`NodeStatus.h` (`_NODE_STATUS` proto v2 + `NodeFaultCode`), SkyHawkClient (host surfacing).

---

## D14 — Node status/fault: a neutral NodeStatus library + FaultSource registry (#163)

**Decision:** the cross-node **status** contract lives in a neutral `NodeStatus` library —
`NODE_STATUS_*` (the DCS-BIOS host-reporting contract), `NodeHealthFlag` (HEALTH_n flag bits),
`NodeFaultCode` (the wire faultId dictionary), and `FaultSource` (the interface a fault-producing
object implements). A fault-producing object (e.g. `DrumDisplay`, a future PDU rail monitor, a
PanelBridge host-link watchdog) **inherits `FaultSource`** and self-registers. A node-level
aggregator (`OpenSkyhawk::aggregateFaults()`, implemented in PR-3) walks `FaultSource::head()`,
picks the primary `NodeFaultCode` (first non-`NONE` in registry order) for `HEALTH_n.faultId`,
derives DEGRADED, and logs `faultDetail()` to **DiagSerial only** (edge-logged per node).
`NodeHealthPayload` (the CAN frame struct) stays in `CANProtocol.h`.

**Rationale:**
- *It's node **status**, not just health, and every node type has it* — PanelGroup, PanelBridge,
  and the PDU (#202) are all nodes. The vocabulary can't be shaped around one flavor (`PanelGroup`/
  `OutputBase`) or the HID namespace, and it's broader than "health" (it owns the `_NODE_STATUS`
  reporting layer). A neutral `NodeStatus` library every node depends on is the right home; the
  client syncs the whole contract from one place.
- *Fault sources feed a node aggregator; no producer "owns" node health.* `DrumDisplay` is **one**
  fault source ("I2C peripheral fault"), not special. A PDU reports over/under-voltage/short; a
  bridge reports host-link-lost. Modeling each as a `FaultSource` (self-registering, reporting
  cached `faultCode()` + local `faultDetail()`) keeps output/monitor classes decoupled and lets the
  fault vocabulary grow per node type. `OutputBase` gains **no** fault API.
- *Coarse, one-at-a-time faultId on the wire; device-detail strings stay local (DiagSerial), never
  over CAN and never in a shared/HID header.*

**Affects:** `NodeStatus.md` (new lib), `02-can-protocol.md`, `04-dcs-bios-integration.md`,
`05-panelgroup-api.md` (`OutputBase` has no fault API), `CANProtocol.h` (keeps `NodeHealthPayload`),
`DrumDisplay` (a `FaultSource`), SkyHawkClient#40 (fault-label table).

---

## D15 — ADC prescaler `/6` on spec grounds, not on a measured symptom (#263)

**Decision:** `STM32Board`'s strong `SystemClock_Config` sets `RCC_ADCPCLK2_DIV6` (APB2 72 MHz →
**ADCCLK 12 MHz**) on **both** of its exit paths. Previously nothing in the project wrote `ADCPRE`,
so it sat at its reset `/2` and the ADC ran at **36 MHz against the F103's 14 MHz maximum**. `/4` =
18 MHz is still out of spec, so `/6` is the fastest legal divider.

We are recording this as a decision **because the obvious justifications turned out to be false**,
and without this record the next person will re-derive and re-bench them.

**Rationale:**
- *Nobody owned the setting, and that is a structural fact, not an oversight to re-litigate.* On F1
  the STM32duino core deliberately delegates the ADC prescaler to `SystemClock_Config` (its
  `__HAL_RCC_ADC_CONFIG` is guarded `!defined(STM32F1xx)`), and `variant_generic.cpp` — the variant
  our #245 override displaces — never set it. `variant_PILL_F103Cx.cpp` *does*, via HAL, so this
  change is restoring a step the override dropped rather than inventing one. Use HAL
  (`HAL_RCCEx_PeriphCLKConfig`) to match both that variant and the rest of `STM32Board.cpp`.
- *Both exit paths, not one.* `SystemClock_Config` returns early on the verified 72 MHz path and
  falls through to an HSI-8 MHz fault path. A single write at the early `return` would leave
  `ADCPRE` at reset for every faulted board. Covered by `test_adc_clock` / `test_adc_clock_fallback`.

**Three predicted impacts were measured on the assembled PanelGroup Rev 1 and found ABSENT:**

| Predicted | Measured at 12 vs 36 MHz |
|---|---|
| Axis noise | 0 vs 1 emission per 100 s; smoothed p-p 24 vs 25 counts. Buffered hall outputs settle fine in the 375 ns that 13.5 cycles gives at 36 MHz. |
| PA2 under-settling | 4.3 % short of rail (177 LSB) at 36 MHz is real — but PA2 is a button, read digitally in production. |
| Die temperature | Predicted corrupt (sensor needs ≥17.1 µs; 239.5 cycles = 6.65 µs at 36 MHz). Read **34.0 °C at both clocks**; Vdd 3206 vs 3203 mV. |

An earlier "~3× noise improvement" figure came from a **breadboard**, whose contact resistance was
acting as a high source impedance. It does not transfer to a properly-wired board.

Rationale, continued:
- *So the honest justification is narrow:* the part was running 2.6× over its rated maximum, and
  high-impedance sources are susceptible in general — cockpit potentiometers sit at ~2.5 kΩ at
  mid-travel and should behave like PA2 by the same arithmetic. **That has never been measured on an
  actual pot.** No shipping analog input is known to be affected today. Do not let this change be
  described as fixing anything observable.
- *If someone wants to settle the pot question,* `Examples/E2E_DCS_Test/PanelGroupInputs` already
  wires ARC-51 VOL — a real pot — to PA2 as an `AnalogInput`; `Tests/PinRef/test_gpio_analog` on its
  10 kΩ + 10 kΩ divider is a second rig. Sample two channels, not one: a single channel leaves the
  sample-and-hold already at the right voltage, so settling is never stressed.
- *Cost accepted:* conversions take 3× longer (26 cycles: 0.72 µs → 2.17 µs; `analogRead()` ~56.5 µs
  → ~63 µs end-to-end, since its per-call calibration triples too). Irrelevant against
  `AnalogInput::POLL_MS` = 8 ms. `ADCPRE` feeds only the ADCs, so #245's CAN timing is untouched.

**Affects:** `08-hardware-firmware-contracts.md`, `STM32Board.md` (TechSpec — new *ADC clock
configuration* section, `begin()` diag-line format), `AnalogInput.md` (acquisition-time budget),
`STM32Board.cpp` (`_configAdcClock`), `Firmware/Tests/STM32Board` (two new envs).

---

## D16 — Control classes: two groups, class families, DCS-BIOS names (#286)

**Decision:** control classes sit in two groups — **Inputs** (`InputBase`) and **Outputs**
(`OutputBase`). Inside a group, related classes form a **family**: one base class holds the shared
behaviour, and thin subclasses change only the **source** (inputs) or the **sink** (outputs). The
precedent is `MultiPosInput` → `SwitchMultiPos` / `AnalogMultiPos`. Class names and behaviour follow
`dcs-bios-arduino-library` wherever an equivalent exists; where a class deliberately differs, its
TechSpec says why.

| Family base | Members | DCS-BIOS equivalent |
|---|---|---|
| `RotaryEncoder` | `RotaryAcceleratedEncoder` — momentum filter + `fastStep`; REL constructor (filter + speed) and DIR constructor (filter only) | `RotaryEncoder` / `RotaryAcceleratedEncoder` |
| `AnalogInput` | `AngleSensorInput` — angle meaning (degrees, 0°/360° wrap) on a `PinRef` source, through a protected virtual `readRaw()` | `Potentiometer` (no angle-sensor class) |
| `AnalogOutput` (abstract) | `Dimmer` (PWM duty), `IntegerOutput` (user callback); `ServoOutput` optional later | `Dimmer` / `IntegerBuffer` / `ServoOutput` |
| `Switch2Pos` | `SwitchWithCover2Pos` — one pin; sequences the cover before the switch | `SwitchWithCover2Pos` |

**Not ported: `DcsBios::RotarySwitch`.** It assumes position 0 at boot and sends absolute
`set_state` values, so the first click after a cold start can jump the sim away from a mission
preset. `RotaryEncoder` in DIR mode drives the same bounded selectors statelessly — DCS owns the
position and clamps at the ends — and DCS-BIOS's own M2000C radio examples use `RotaryEncoder`
INC/DEC for exactly those knobs.

**Rationale:**
- One decode / filter / emit path per family means one set of tests. A subclass is a constructor
  plus at most one override, so it adds nothing to the ISR or the loop hot path.
- Constructor length: `RotaryEncoder` already takes six positional arguments; putting the
  acceleration options on it would make eight, easy to pass in the wrong order. A subclass
  constructor makes the required arguments explicit (`fastStep` is required, `mode` is implied).
- DCS-BIOS parity lowers the barrier for builders who already know that library — the original
  plan was to "use dcs-bios-arduino-library as much as possible".

**Affects:** `05-panelgroup-api.md`, `10-implementation-plan.md`, TechSpec
(`RotaryAcceleratedEncoder`, `AngleSensor`, `AngleSensorInput`, `AnalogOutput`, `Dimmer`,
`IntegerOutput`, `SwitchWithCover2Pos`; `RotarySwitch` removed), `docs/firmware/control-types.md`.

---

## D17 — Node status traffic is owned by CANProtocol, not by each node library (#340)

**Decision:** The duties every node owes the bus — `HB_n` every 500 ms, `HEALTH_n` every 1000 ms
(with the fault edge log and the opt-in overheat WARNING), and the master-loss watchdog on `HB_0` —
run in one place, `CANProtocol::serviceNodeStatus()`, which each node calls once per `loop()`.
PanelGroup and PanelBridge no longer carry their own copies.

**Rationale:**
- *They were copies, not choices.* Searching the history found no recorded reason for the heartbeat
  living in PanelGroup / PanelBridge: it came with the 2026-05-21 library split of the prototype loop,
  `HEALTH_n` (#213) was added beside it in both places, and the `HB_0` watchdog (#93) went into
  PanelGroup because PanelGroup was then its only consumer.
- *A third node type made the cost real.* The PDU monitoring node (#216) has no inputs or outputs.
  It needs exactly this plumbing and nothing else from PanelGroup; a third copy, or a dependency on
  the panel library, would both be wrong.
- *CANProtocol is the lowest layer that can host it.* STM32Board sits below CANProtocol
  (`CANProtocol` calls `STM32Board::canHandle()`), so STM32Board cannot send frames without a
  circular dependency. CANProtocol already builds both payloads and already handles the other
  node-generic frames in `drain()` (`SYNC_REQ`, the `TEST_SEQ` echo).
- *Behaviour is preserved, timing included.* The service arms its timers on its first call — the
  first `loop()` after `setup()` — so the first `HB_n` still goes out ~500 ms after boot completes
  (09 step 8), as PanelGroup's old step-8 arming did; `HB_0` is still passed to `onReceive()` after the watchdog
  stamps it. The move also removes a latent #225-pattern false timeout in the old PanelGroup
  watchdog, which compared a `now` sampled *before* `drain()` against an `HB_0` stamped *during* it.

**Not moved:** node-domain work stays where it is — PanelGroup's `rxCount` (accepted `CTRL_BCAST`),
`READY_n`, input/output dispatch and `SYNC_REQ` handling; PanelBridge's node tracking.

**Affects:** `02-can-protocol.md`, `06-panelbridge-api.md`, `08-hardware-firmware-contracts.md`,
`09-startup-resync-diagnostics.md`, `11-open-issues.md`, TechSpec (`CANProtocol`, `PanelGroup`,
`PanelBridge`, `STM32Board`), `docs/architecture/can-bus.md`.

---

## D18 — PDU telemetry on a paged TELEM_n frame, forwarded unread (#216)

**Decision:** node telemetry beyond generic health travels on one new per-node frame family,
`TELEM_n = 0x180 + n`, whose byte 0 is a global, append-only `TelemPage` naming the layout of the
other seven bytes. PanelBridge forwards every `TELEM_n` frame to the host **unread** as
`_NODE_TELEM <nodeId><8 bytes>`, versioned on its own (`NODE_TELEM_PROTO_VERSION`) so `_NODE_STATUS`
stays at v2. The first pages belong to the PDU monitoring node: `PDU_RAIL_12V`, `PDU_RAIL_5V`,
`PDU_STATUS`, sent one frame per second in rotation plus rate-limited edges on any flag change.

**Rationale:**
- *Layer on HEALTH_n, don't duplicate it.* `HEALTH_n` already is the generic all-node health frame
  (die temp, degraded, faultId). PDU rail faults reach it through the FaultSource registry (D14) like
  any fault; only the rail values and their detail flags need a new frame.
- *A page byte, not fixed IDs.* Per-node ID blocks cost 64 IDs each, and the original proposal's
  `0x700 + n` had meanwhile gone to `EVT_ACTION_n` (#116). One family with a page byte keeps the ID
  map small and lets the bridge stay ignorant of layouts — a new page needs no bridge change.
- *Engineering units on the node.* The PDU holds its own per-board calibration, so it sends mV / mA /
  0.1 °C; the bridge forwards bytes and the client needs no per-board knowledge. Raw ADC counts stay
  on DiagSerial for bring-up.
- *One frame per second.* Matching the `HEALTH_n` rate keeps `_NODE_TELEM` traffic to the host — and
  the DCS relay, which receives every device line — low (about 3 messages/s for three PDUs), and the
  CAN load negligible (~0.08 %). Edge sends make faults immediate anyway, where the 5 s roster poll
  would delay them.
- *VDDA: nominal until anchored.* The F103's Vrefint is untrimmed (1.16–1.24 V, ±~3 %), a worse
  reference than the AMS1117's ±1–2 % nominal, so an uncalibrated PDU assumes 3300 mV; a one-time
  `CAL VDDA <mV>` (DMM on the 3V3 rail) stores this chip's Vrefint, after which VDDA tracks it.

**Affects:** `02-can-protocol.md`, `04-dcs-bios-integration.md`, `06-panelbridge-api.md`,
`08-hardware-firmware-contracts.md`, `10-implementation-plan.md`, TechSpec (`CANProtocol`,
`NodeStatus`, `PanelBridge`, `PowerMonitor`), `docs/architecture/can-bus.md`, `docs/architecture/power.md`.

---

## D19 — Every board announces its kind; nodes detect a duplicate NODE_ID (#344)

**Decision:** `HEALTH_n` byte 5 (reserved until now) carries a `NodeKind` — `0x00` unknown, `0x01`
panel group, `0x02` PDU, `0x03` bridge, append-only in `NodeStatus.h` — set by the board's firmware
role: `serviceNodeStatus(kind, …)` takes it as a required argument and each node library passes its
own. PanelBridge caches it and forwards it in `_NODE_STATUS` **v3** (28 hex), together with a node-0
row for itself (its heartbeat state + its own `HEALTH_0`, kind bridge), and pushes a delta when an
alive node's kind changes. Separately, every node filters its own `HB_n`; a frame on it raises
`NodeFaultCode::DUPLICATE_NODE_ID`, ahead of every `FaultSource`, clearing 3 s after the last
foreign frame.

**Rationale:**
- *The ID is not a type.* A PDU can take any `NODE_ID`, and silence proves nothing (a panel group
  sends no telemetry), so the client had no positive signal for a panel group and couldn't let one
  be adopted. Only the board's own firmware knows what it is.
- *A required argument, from the library.* Sketches never call `serviceNodeStatus()` — the node
  library does — so the library is the role. Making `kind` required means a new node library won't
  compile until it says what it is; nothing ambiguous needs a per-sketch override today.
- *Byte 5 is backward-safe.* Pre-#344 firmware already sends 0 there, which reads as unknown — an old
  node stays un-adoptable until re-flashed, which is intended. The proto bump makes a v2 client fail
  loudly instead of misreading 28 hex as 26.
- *The bridge reports itself.* Every board reports, for consistency; the bridge row is the client's
  own link, never a new node. The bridge stays on node 0 — the master address every node watches.
- *Kind delta.* The alive delta leaves on a node's first `HB_n`, ~1 s before its first `HEALTH_n`,
  so without a push the kind would wait for the client's next roster poll.
- *Duplicate IDs hide.* Two boards on one ID blend into one node and collide on identical CAN IDs
  (error frames, error-passive), while inputs mostly still work — a "flaky board". Filtering one's
  own HB costs one filter slot and no traffic. The duplicate outranks every other fault because
  nothing reported under a shared ID can be trusted, and it auto-clears so fixing the other board
  needs no reboot of this one. It is not a registry `FaultSource`: registry order is construction
  order, which across translation units can't guarantee its priority.

**Affects:** `02-can-protocol.md`, `04-dcs-bios-integration.md`, `06-panelbridge-api.md`,
`10-implementation-plan.md`, TechSpec (`CANProtocol`, `NodeStatus`, `PanelBridge`, `PanelGroup`,
`PowerMonitor`), `docs/architecture/can-bus.md`, `docs/firmware/node-id.md`.

---

## D20 — SimGateway never blocks on the host, and recovers its own USB link (#355)

**Decision:** The PanelBridge→host relay is line-granular. A line of printable ASCII + `\n` is
written whole when the CDC FIFO has room, otherwise dropped and counted; non-ASCII and overlong
lines are dropped. Calibration frames are whole-or-drop too. A FAULT latched 3 s restarts the UART
and clears itself. A pure recovery ladder (`LinkRecovery.h`) re-attaches USB after 5 s unhealthy
(unmounted, suspended with a recent UART fault, or a CDC IN endpoint that stopped draining),
reboots 5 s later if that fails, then backs off to a re-attach every 60 s. A 3 s hardware watchdog
covers a hung `loop()`. Link-health counters and event times are exposed as `GET_DIAG` / `DIAG` on
the calibration protocol.

**Rationale:**
- *The failure couldn't be reproduced, so the fix can't depend on knowing the trigger.* Twice the
  RP2040 dropped off the bus with its core running (FAULT flashing) and only a power cycle
  recovered it. Recovering whatever the cause, and recording which cause it was, beats guessing.
- *A blocking write is a latent hazard on its own.* Adafruit TinyUSB's `write()` loops while the
  host holds DTR but stops reading, and the old per-byte relay called it unguarded. It did not
  cause the observed failure (the loop was still animating the LED), but it could.
- *Lines, not bytes, are the unit to drop.* Everything PanelBridge sends the host is a DCS-BIOS
  message; dropping a whole one loses one command, while truncating one changes it. Buffering lines
  also means a calibration frame can no longer land inside a line.
- *FAULT is electrical, not starvation.* The core's UART RX IRQ empties the PL011 into a software
  queue, so a slow `loop()` overflows that queue silently and never sets RSR. That queue is now
  1 KB (~40 ms) and counted separately; the RSR bits are counted one by one.
- *Mounted is the main health signal; suspend counts only with a recent UART fault.* The RP2040
  is powered only from USB, so running-but-unmounted is abnormal, while a sleeping host suspends a
  device that is perfectly healthy.
- *A stalled endpoint is unhealthy too.* Once writes never block, a host that stays configured but
  stops polling would turn the old hang into a silent permanent outage — `loop()` and the watchdog
  keep running. DTR held + bytes pending + nothing drained for 1 s catches it; the host OS polls an
  open port into its own buffer regardless of the app, so a slow app does not trip it.
- *Backoff, not a reboot loop.* A Gateway on a charger or back-fed with no host would otherwise
  reboot every 10 s.
- *Diagnostics on the existing binary channel.* `GET_DIAG` is answered on request and adds nothing
  to the stream unasked; it is additive, so `CAL_PROTO_VERSION` stays 1.

**Affects:** `03-uart-usb-hid-protocol.md`, `07-simgateway-api.md`, `08-hardware-firmware-contracts.md`,
`09-startup-resync-diagnostics.md`, TechSpec `SimGateway.md`, `docs/architecture/sim-gateway.md`,
`docs/firmware/debugging.md`. Client half: SkyHawkClient sync of `CalDiagField`.

---

## D21 — Every node announces its firmware version in READY_n (#295)

**Decision:** `READY_n`, empty until now, carries a `ReadyPayload`: bytes 0–2 = the firmware
release version (`OPENSKYHAWK_FW_*` from the header-only `FirmwareVersion` library, rewritten by
release-please), bytes 3–7 reserved. Nodes send it through `CANProtocol::sendReady()`. PanelBridge
caches the version per node, keeps it across a timeout, pushes a `_NODE_STATUS` delta when an alive
node's version changes, and reports it in `_NODE_STATUS` **v4** (34 hex), the node-0 row carrying
its own. An empty READY (older firmware) and a READY the bridge never saw both read 0.0.0, "not
reported".

**Rationale:**
- *v1.0 has a mixed-version rule* — nodes on one bus share a major version — and until now nothing
  on the bus said which firmware a board runs. A missed board in a reflash stayed invisible.
- *READY, not HEALTH.* HEALTH's two reserved bytes are kept for future generic health, and TELEM
  never carries health, so spending them on identity would leave health nowhere to grow. READY was
  empty: eight free bytes, no new frame, no extra traffic.
- *Every update is a reboot.* An ST-Link upload resets the MCU, and READY is the last step of every
  boot, so a reflash is announced at once. A node that only lost its CAN link doesn't reboot, and
  its cached version is still right.
- *Empty means older, unambiguously.* Pre-#295 firmware sends READY with DLC 0, so a missing
  payload can't be mistaken for a real version.
- *Accepted gap:* READY is once per boot, so a bridge that boots after a node reads it as not
  reported until that node reboots. A re-announce on the bridge's boot `SYNC_REQ` can follow as a
  minor change — `SYNC_REQ` is empty today.
- *One version source.* The same header feeds SimGateway's `HELLO_ACK`, so the Gateway's two MCUs
  can't report different versions for one build.

**Affects:** `02-can-protocol.md`, `03-uart-usb-hid-protocol.md`, `04-dcs-bios-integration.md`,
`06-panelbridge-api.md`, `09-startup-resync-diagnostics.md`, TechSpec (`CANProtocol`, `NodeStatus`,
`PanelBridge`, `PanelGroup`, `PowerMonitor`, `README`), `docs/architecture/can-bus.md`,
`docs/architecture/firmware-tiers.md`. Client half: SkyHawkClient sync of `NodeStatusField` v4.

---

## D22 — Within a major, the wire only grows and every reader tolerates growth (#295)

**Decision:** From v1.0, CAN frames and payloads, the host `_NODE_*` lines and their enums are
frozen per major. Writers only append: a frame never grows (8 bytes), a field never moves, resizes
or changes meaning; new data goes into reserved bytes, a new `TelemPage`, or a new frame family in
a free ID range, and new enum values are allowed. Readers accept a longer `_NODE_STATUS` row,
parse the fields they know and ignore the rest, keep unknown enum values as "unknown (0xNN)", and
ignore reserved bits. A shorter row means older firmware and is reported, not dropped silently.

**Rationale:**
- *A strict reader makes every addition a major.* The client dropped any `_NODE_STATUS` row whose
  length wasn't exactly 28, so the first 1.x field (as #344's `nodeKind` and #295's version were)
  would have blanked every 1.0 client's node list.
- *Growth paths are named so they don't collide.* Health stays in `HEALTH_n` (its two reserved
  bytes), node telemetry in `TELEM_n` pages, identity in `READY_n` — each owner kept single.
- *The calibration channel is the exception.* Its `LEN` is fixed per type on purpose (false-frame
  rejection inside the DCS-BIOS stream), so it grows by new message types instead (`03`).

**Affects:** `02-can-protocol.md`, `03-uart-usb-hid-protocol.md`, `04-dcs-bios-integration.md`,
`NodeStatus.h`. Client half: SkyHawkClient's `_NODE_STATUS` parser and alerts (client 1.0).

---

## D23 — The roster request moves to export address 0xFF00 (#295)

**Decision:** The client asks PanelBridge for the node roster by writing DCS-BIOS export address
`0xFF00` (`NODE_STATUS_REQ_ADDR`), not `0x86FE`. It ships with `_NODE_STATUS` v4, which the client
must adopt anyway.

**Rationale:**
- *`0x86FE` was not reserved, only unused by the A-4E.* In DCS-BIOS v0.11.8 it is the AH-64D's
  `PLT_VIDEO_FLIR_GAIN` and the Mirage F1's `P_VOR_01_G`. Flying either with the cockpit connected,
  every export of that control looked like a roster request and drew a full roster burst.
- *`0xFF00` belongs to no module.* Every aircraft module ends by `0xB2FB`; DCS-BIOS's own
  `MetadataEnd` sits at `0xFFFE`. Module blocks are allocated upward from low addresses, so the top
  of the map is the safest place for a host-only address.
- *Now, not later.* The request address is part of the v1.0 host contract; moving it after the
  freeze would be a major.

**Affects:** `NodeStatus.h`, `04-dcs-bios-integration.md`, `06-panelbridge-api.md`, TechSpec
`NodeStatus`, PanelBridge README, `docs/architecture/sim-gateway.md`, `docs/architecture/firmware-tiers.md`.
Client half: SkyHawkClient's roster request (synced from `NodeStatus.h`).

---

## D24 — PanelBridge forwards the whole DCS-BIOS export stream (#295)

**Decision:** `BridgeExportListener` covers `0x0002`–`0xFFFE`: every export address of whatever
aircraft is flying, plus CommonData and the DCS-BIOS metadata, goes onto CAN as `CTRL_BCAST`. The
nodes decide what they use — each output matches its own address. Only `0x0000` is skipped: it is
the `ControlPacket` empty-slot sentinel, so CAN can't carry it (it holds the first two characters of
`_ACFT_NAME`).

**Rationale:**
- *The old window was the input-ID range by accident.* It was `0x8000`–`0x86FF` — the `DCSIN_*`
  range — not the A-4E's export block (`0x8400`–`0x8555`), so it already passed parts of the AH-64D,
  MB-339 and Mirage F1. Filtering by aircraft would tie the bridge to one aircraft; the multi-aircraft
  direction wants the cockpit, not the bridge, to choose.
- *Nodes already ignore what they don't use.* `onControlPacket` hands every packet to every output,
  and each keeps only its own address.

**Cost — confirm on the #290 soak:** `CTRL_BCAST` (the highest-priority frame) now also carries
CommonData (position, heading, model time) and the per-frame update counters, so bus load rises.
The soak's "no false offline / zero TX drops" checks are the guard; `CTRL_BCAST` keeps the newest
state when its queue is full.

**Affects:** `PanelBridge.cpp`, `02-can-protocol.md`, `06-panelbridge-api.md`, `10-implementation-plan.md`,
TechSpec `PanelBridge`, PanelBridge README, `docs/architecture/can-bus.md`.

---

## D25 — Every production image carries a stamped identity block (#362)

**Decision:** Each production STM32 image has a 32-byte identity block at image start + `0x200`:
magic, layout version, size, image length, image CRC, NODE_ID, NodeKind and the firmware version.
The role library defines it; a post-build step stamps the length and CRC (CRC-32/MPEG-2 over 32-bit
words, gaps filled 0xFF). The block carries the version only — no commit id. Layout in `08`.

**Rationale:**
- *The image must say what it is before it is sent.* The uploader and the client refuse a file for
  the wrong board kind or the wrong NODE_ID, and show the version it holds, without flashing it or
  asking the board. A raw `.bin` has no metadata of its own.
- *The bootloader needs a cheap "is the app whole?" check.* Length + CRC in the image lets it reject
  an interrupted write at every boot. CRC-32/MPEG-2 over words is what the F103's CRC unit computes,
  so the bootloader verifies 60 KB in hardware without a table.
- *Fixed offset, not a symbol.* Readers (bootloader, uploader, client) find it without an ELF, from
  the flat image or flash. `0x200` clears the F103 vector table (`0x10C`) with room to spare and
  moves with the image when the bootloader shifts the app.
- *0xFF gaps.* An ST-Link flash leaves gaps erased (0xFF) while `objcopy` fills them with 0x00, so
  without the fill the `.bin`, the `.elf` and the board would give three answers for one image.
- *Version only.* A release build is cut from its tag, so the version names the commit; dev builds
  keep the last release's version (accepted). A commit id would only be known after the commit.
- *Per-role, not per-sketch.* NODE_ID is already a build flag and the role fixes the NodeKind, so a
  sketch author writes nothing. `Tests/` builds keep the default linker script — the block is
  dropped there.

**Affects:** `08-hardware-firmware-contracts.md`, TechSpec `FirmwareImage` + `README`,
`Firmware/shared/` (linker script, `stm32.ini`, `osh_image.py`), the role libraries, every
production `platformio.ini`, `docs/firmware/platformio-setup.md`. Consumers: #363 (bootloader),
#366 (uploader), SkyHawkClient#177.

---

## D26 — Our own 4 KB CAN bootloader on every STM32 board (#363)

**Decision:** Every STM32 board carries our own bare-metal bootloader in its first 4 KB; apps start at
`0x08001000`. It starts only an app whose identity block and CRC check out, takes an image over CAN on
`BOOT_CMD_n` / `BOOT_RSP_n` with block 0 committed last, and keeps the last valid app's NODE_ID and kind
in option bytes Data0 / Data1. Blank boards get bootloader + app in one first flash (`-t bootstrap`, the
release's `-full.hex`). Protocol and boot flow: `12-bootloader.md`.

**Rationale:**
- *Updates without an ST-Link.* The client updates boards over the cockpit bus (#366,
  SkyHawkClient#175); every board must be able to take an image, so the bootloader is on all of them —
  the Arduino model, not an option.
- *Our own, not Katapult.* Katapult (#361 spike) checks only the first word of the app, writes the vector
  table first (a half-written image passes), has no timeout, uses CAN IDs that collide with
  `HB_n` / `HEALTH_n` / `TELEM_n`, and is GPLv3 with a make/Kconfig build. Ours is ~3.1 KB bare-metal
  CMSIS, in PlatformIO, on the IDs #295 reserved.
- *4 KB, not less.* The F103 write-protects in 4 KB blocks and #364 protects the bootloader; a smaller
  bootloader would leave the rest of its block unusable anyway.
- *Commit last + CRC at every boot.* With block 0 held back, an interrupted transfer — power pulled at
  any block — leaves no valid vector table; the identity-block CRC catches everything else. The
  hardware CRC unit checks 60 KB in well under a millisecond.
- *NODE_ID in the option bytes.* The bootloader must answer as its node when the app is broken —
  exactly when it is needed. The build stays the source (D10); the option bytes mirror the last valid
  app, so an ST-Link repurpose follows automatically. Written only when they differ, because the
  option-byte erase is the one moment a power cut costs an ST-Link unlock.
- *One wire contract.* `BootProtocol.h` is the single source for the bootloader, CANProtocol, the
  uploader and the client sync (#346 named values).

**Affects:** `02-can-protocol.md`, `08-hardware-firmware-contracts.md`, `09-startup-resync-diagnostics.md`,
`12-bootloader.md`, TechSpec `Bootloader` + `BootProtocol`, `Firmware/shared/` (layout, `bootstrap`),
the release workflow + READMEs, `docs/firmware/bootloader.md`, `docs/firmware/platformio-setup.md`,
`docs/guides/flashing.md`, `docs/architecture/can-bus.md`. Follow-ups: #364 (safety), #365 (app +
bridge), #366 (uploader).

---

## D27 — The bootloader guards itself and its board's identity (#364)

**Decision:** The bootloader write-protects its own 4 KB on first boot; refuses an upload whose NODE_ID
or kind differs from the board's (`SET_NODE_ID` is the separate, explicit way to change it); stays in
charge after `ATTEMPT_LIMIT` (3) app boots that never report healthy (backup-register counter, cleared by
`STM32Board::markBootHealthy()`); listens 200 ms after every reset for a `CONNECT` (catch at boot);
returns to a still-valid app after 30 s idle; and lets a board with no NODE_ID take its first image over
CAN, addressed by a hash of its chip UID. Details: `12-bootloader.md`.

**Rationale:**
- *Self-protection, not a flashing step.* A field board can't be bricked by a stray write to the first
  4 KB, and nobody has to remember to set WRP — the bootloader does it on its first boot, through the
  option-byte routine it already has for the NODE_ID copy. The cost is a documented unprotect line
  before replacing the bootloader or flashing a per-library test at `0x08000000`.
- *NODE_ID is enforced where it can't be skipped.* The client checks too, but the bootloader is the
  last word: a mis-picked file can't turn node 3 into node 5's twin. An ST-Link flash stays the
  exception — physical access is the authority (repurpose).
- *Crash loop before catch at boot.* An app that resets before it is healthy is caught by count, with
  no host involved; an app that runs but ignores the bus (CAN dead, hung) needs a host sending CONNECT
  across a reset — the 200 ms window, the price of which is 200 ms on every boot.
- *The app-side clear ships with the counter.* Planned for #365, it moved here: a counter without its
  clear would stop every board after three resets.
- *UID hash in every announcement.* Two spare bytes identify an unconfigured board (one on the bus at a
  time) and expose two boards claiming one NODE_ID, without a second frame type.

**Affects:** `12-bootloader.md`, `02-can-protocol.md`, `BootProtocol.h`, `Firmware/Bootloader`,
`STM32Board` (`markBootHealthy()`), PanelGroup / PduNode / PanelBridge, TechSpec `Bootloader` +
`BootProtocol` + `STM32Board`, `docs/firmware/bootloader.md`, `docs/guides/flashing.md`. Client half:
SkyHawkClient#175 (stuck / unstable states), #177 (From file…, change-node-ID-first).

---

## D28 — Updates ride the existing host link: export records down, `_BOOT_RSP` up (#365)

**Decision:** The host reaches a board's bootloader through the Gateway and PanelBridge with no new
framing on either link. Down, each bootloader CAN frame is a DCS-BIOS export record at the reserved
window (`0xFF10`, stream-encoded since #366 — D29); up, each bootloader frame is a `_BOOT_RSP` DCS-BIOS
message. A running app enters its bootloader on a CONNECT, only when the bootloader's info block
(`0x08000FF0`) says one is installed. `_NODE_STATUS` gains `bootloader` and `bootState` inside v4.

**Rationale:**
- *The Gateway → bridge UART is the raw export stream.* Any byte can appear in it, so a magic-byte
  frame can't be told apart — but reserved export addresses already work for the roster request
  (D23). One byte per word because the DCS-BIOS parser resyncs on four `0x55` in a row, which image
  data could contain.
- *Today's Gateway carries it unchanged.* It relays export bytes down and whole printable lines up,
  so #365 was benched from a PC before the Gateway uploader (#366) existed, and #366 drives the same
  format from the RP2040.
- *The bridge stays dumb.* It relays frames and reports state; the uploader owns sessions, retries
  and checks. It passes only the boot command IDs, so the host can't inject other CAN traffic.
- *CONNECT means "enter".* The host's loop is the same whether the board runs its app, is in its
  bootloader, or is being caught at reset.
- *Detect, don't assume.* An app linked behind the bootloader could still sit on a board whose
  bootloader is older or absent (bench); reading the info block makes "ignore the request" exact, and
  gives READY_n byte 3 the installed version, not the one the app was built against.
- *Inside v4.* v4 hadn't shipped and the client wasn't on it yet, so appending the two fields needed no
  bump.

**Affects:** `BootProtocol.h`, `NodeStatus.h`, `CANProtocol` (filter, drain, READY), `STM32Board`
(`bootloaderVersion()`, `enterBootloader()`), `PanelBridge`, the bootloader (info block, LEDs, idle
announce), `02`, `04`, `06`, `09`, `12`, TechSpec, `docs/firmware/debugging.md`,
`docs/firmware/bootloader.md`. Client half: SkyHawkClient#175.

---

## D29 — The Gateway is the uploader, and the transfer streams (#366)

**Decision:** The Gateway's RP2040 stages an image the client streams over USB (`UPL_*` on the calibration
envelope, `03`), then flashes the board itself through PanelBridge and finishes on its own: it checks NODE_ID,
kind and size from the bootloader's answer before anything is erased, retries the whole flash twice after a
transport failure, and confirms success by the board's `_NODE_STATUS` row reporting the image's version. The
transfer streams: the bootloader erases the image's pages at BEGIN and buffers 4 blocks in flight (go-back-N
on `BLOCK_ACK.next`), and the relay packs whole frames per export record with `0x55` / `0x56` escaped.

**Rationale:**
- *The client may go away.* A laptop sleeping or a USB cable pulled mid-flash must not leave a board half
  written with nobody to finish it — the Gateway holds the image and the state, and reports the result when
  the client comes back (`UPL_INFO_ACK`).
- *No USB round trip per block.* Driving the protocol from the RP2040 takes the PC out of the per-block loop;
  the client's own staging is a plain bulk copy.
- *Speed had to be settled before the freeze (#367).* Stop-and-wait over the relay ran ~2.7 KB/s — bounded
  by four UART bytes per data byte and one block in flight. Streaming lives in the bootloader, so it went in
  now: ~0.25 KB more code, protocol version unchanged (nothing had shipped). With the denser relay
  (~2 UART bytes per data byte, bridge firmware — still changeable later) the Gateway writes at ~12.5 KB/s:
  a 60 KB board in ~6 s, a full cockpit in about a minute.
- *Decide before erasing.* ID / kind / size checks use the bootloader's CONNECTED answer, so a wrong file
  never costs a board its app.

**Affects:** `BootProtocol.h` (window, `BLOCK_ACK.next`, relay encoding), `Firmware/Bootloader`
(erase-at-BEGIN, RX ring, mailbox wait), `PanelBridge` (stream decoder), `SimGateway` (`Uploader`,
`UploadProtocol.h`, `AxisCal.h` types / NACKs / inbound size), `03`, `04`, `06`, `07`, `12`, TechSpec,
`docs/architecture/sim-gateway.md`, `docs/firmware/bootloader.md`. Client half: SkyHawkClient#175–#177.

---

## D30 — The bridge updates over USART2 from its own bootloader build; the bootloader freezes (#367)

**Decision:** The Gateway's PanelBridge — the CAN master, which nothing on the bus can update — gets a second
build of the bootloader (`Firmware/Bootloader` env `bridge`, `-DOSH_BOOT_BRIDGE`) that speaks the same
protocol as `0xAA` frames on USART2 to the RP2040 instead of CAN. The Gateway firmware carries the PanelBridge
app it was built with and flashes it as `UPL_FLASH` target `0`. The info block gains a `transports` byte, and an
app enters only a bootloader that speaks the transport the request came on. From 1.0.0 the bootloader is
frozen: protocol version 1 fixed, the build's platform / toolchain / CMSIS pinned, 4 KB forever, and a written
cut order if it ever has to shrink (`12` § Freeze).

**Rationale:**
- *One image was the goal, and missed by ~60 B.* A custom 16-entry vector table and startup with no libc
  init, plus LTO, won back ~280 B; both transports in one image still overflowed. The alternatives were
  cutting validation (never), growing to 8 KB (4 KB of app space on every board), or two builds — the user's
  stated fallback. Every line but the transport is shared, so the two can't drift.
- *The RP2040 is already the uploader (D29)* and already shares a UART with the bridge — no new wiring, no
  RP2040 → STM32 reset line (the Gateway board has none: NRST goes only to SWD and the reset button).
  Catch-at-boot covers a bridge app that won't enter: the uploader keeps sending CONNECT, the user presses
  reset.
- *Bundle, don't fetch.* A Gateway release always carries the matching cockpit side, so the client can bring
  it up to date with nothing to download, and `bridgeFw` tells it when to.
- *Wrong build, no brick.* An app checks `transports` before resetting, so a board with the other build ignores
  the request and keeps running (fix over ST-Link).
- *Frozen means reproducible.* Release CI rebuilds the bootloader from source; a newer compiler can move its
  size by ±100 B. Pinned, the same source always builds the same bytes; a link that grows into the info block
  fails rather than ships.

**Affects:** `Firmware/Bootloader` (startup, USART2 + DMA transport, `bridge` env, pins), `BootProtocol.h`
(`transports`, serial framing, `BRIDGE_ENTER_EXPORT_ADDRESS`), `STM32Board::enterBootloader(via)`,
`CANProtocol`, `PanelBridge` (`BridgeEnterListener`), `SimGateway` (`Uploader` target 0, UART hand-over),
`Firmware/shared/bundle_bridge.py` + `osh_image.py` (`bootloader_env`), the release workflow, `03`, `04`, `06`,
`07`, `12`, TechSpec, `docs/firmware/bootloader.md`, `docs/architecture/sim-gateway.md`, the flashing guide.
