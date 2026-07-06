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

**Rationale:** `Joystick.use16bit()` expects 0–65535. DCS-BIOS `set_state` controls with
`max_value > 1` also expect 0–65535 (or integer strings thereof). Using one resolution
throughout eliminates per-destination scaling code and makes CAN traffic predictable.

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
≤ 0x7F is forwarded immediately to USB CDC.

**Affects:** `03-uart-usb-hid-protocol.md`.

---

## D7 — CAN TX queue overflow: frame-type dependent

**Decision:** CAN TX overflow policy depends on frame type:

| Frame type | Policy |
|------------|--------|
| `CTRL_BCAST` DCS output broadcasts | Drop stale queued state and retain the newest frame. Coalesce by `controlId` where practical. |
| `EVT_n` input events | Bounded retry: keep the event for up to 3 send attempts, then drop it and increment the `DIAG_ERR` TX drop counter. Never build an unbounded backlog. |
| `HB_n` heartbeat | Drop stale heartbeat; the next heartbeat replaces it. |
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

**Rationale:** STM32 ADC tops at 65520 after ×16 scaling; ADS1115 tops at 65534 after ×2
scaling. `0xFFFF` (65535) is physically unreachable on both hardware paths. No separate
boolean is needed in the position array.

**Affects:** `05-panelgroup-api.md`.

---

## D9 — RotaryAcceleratedEncoder 4-value scheme (not delta + direction separately)

**Decision:** `RotaryAcceleratedEncoder` encodes both direction and speed into a single uint16
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
covered properly by the PDU's INA226 power telemetry (#202). The overheat flag / status-LED
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
`CANProtocol.h` (`_NODE_STATUS` proto v2 + `NodeFaultId`), SkyHawkClient (host surfacing).
