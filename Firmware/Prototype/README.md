# CAN Bus Integration Test

This prototype validates the CAN bus architecture before any production PCBs are built. The goal is to find bottlenecks and failure modes on the bench — the same way the STM32 USB CDC crash was discovered during the DCS-BIOS debug session (see `Docs/claude/dcsbios-stm32-debug.md`).

Three experiments are defined below. They are independent and have different wiring, firmware assumptions, and success criteria. Do not treat them as a single continuous test path.

| Experiment | What it tests | Arduino connected? | DCS running? | Status |
|------------|---------------|--------------------|--------------|--------|
| A — UART byte-stream capture | DCS-BIOS traffic volume and burst characteristics | Yes | Yes | Ready |
| B — Synthetic CAN stress | CAN topology, throughput, and saturation limits | Yes | No | Ready |
| C — DCS-BIOS bridge validation | End-to-end DCS state reaching CAN nodes | No (RP2040) | Yes | **Blocked** — see below |

**Why Experiment C is blocked:** The RP2040 Bridge firmware is a transparent byte relay. It passes raw DCS-BIOS bytes to the STM32 master, which expects 4-byte `ControlPacket` structs. Without a parser/translator on the RP2040 (or on the master), the master receives misframed data and emits `[OVF]` alignment errors rather than forwarding real control state to the CAN bus. Experiments A and B are both useful without resolving this, but Experiment C cannot produce meaningful results until a translator is implemented.

---

## Hardware

| Qty | Part | Notes |
|-----|------|-------|
| 1 | Arduino Mega 2560 | Experiments A and B only |
| 3 | STM32F103CBT6 Blue Pill | Must have external 8 MHz crystal |
| 3 | SN65HVD230 CAN transceiver | 3.3 V supply — **not** 5 V |
| 2 | 120 Ω resistors | CAN bus termination |
| 2 | USB-TTL adapters | Adapter 1: STM32 master PA9 (diagnostic tap). Adapter 2: Arduino Serial2 (commands + analytics). |
| — | 24 AWG twisted pair | CANH/CANL + a separate GND wire run alongside |
| — | 1 kΩ + 2 kΩ resistors | Voltage divider: Arduino Serial1 TX (5 V) → STM32 PA3 (3.3 V max) |

**STM32 master pin assignments:**
- PA11 / PA12 — CAN RX / TX (to SN65HVD230)
- PA2 / PA3 — UART2 TX / RX (to Arduino Serial1 in Exp. A/B; to RP2040 in Exp. C)
- PA9 — UART1 TX, diagnostic tap → USB-TTL adapter 1. **Leave PA10 unconnected.**

**Sub-node ID strap:** same binary is flashed to both sub-nodes. At startup the firmware reads PA0:
- Sub-1: wire PA0 to 3.3 V rail → reads HIGH → `node_id = 1`
- Sub-2: leave PA0 unconnected (firmware enables internal pull-down) → reads LOW → `node_id = 2`

**Button debounce:** PB1 on each sub-node is read with a software edge-detect but no debounce delay. Contact bounce may produce extra events. Do not use edge counts as a precision metric in stress tests.

---

## Firmware

| Project | Target | Role |
|---------|--------|------|
| `CAN_Test_Arduino/` | Mega 2560 | Experiments A and B: byte relay, packet injection, analytics |
| `CAN_Test_Master/` | Blue Pill | UART → CAN bridge, error monitor, node watchdog |
| `CAN_Test_SubNode/` | Blue Pill ×2 | CAN sub-node: heartbeat, echo, button event |

Flash with `pio run -t upload` inside each project directory. The Blue Pill `platformio.ini` files include the Chinese-clone JTAG ID override (`CPUTAPID 0x2ba01477`) — no extra flags needed.

### Serial ports

| Port | Baud | Direction | Purpose |
|------|------|-----------|---------|
| Arduino USB (`Serial`) | 250000 | In | DCS-BIOS input (Experiment A). Do not open this port in a terminal that sends keystrokes — DCS-BIOS owns it. |
| Arduino Serial2 (pins 16/17 → USB-TTL adapter 2) | 115200 | In + Out | **Operator console for all experiments.** Send commands here; analytics and status print here. |
| STM32 Master PA9 (→ USB-TTL adapter 1) | 115200 | Out | CAN bus diagnostics. Open this in a second terminal window alongside the Serial2 console. |

### Commands (send one character to Arduino Serial2)

| Key | Mode | Description |
|-----|------|-------------|
| `D` | DCS capture | `Serial` is fixed at 250000 from boot; `D` resets capture statistics for a clean run. Relay continues until `I`. |
| `I` | Idle | Stop all injection and capture. |
| `S` | Slow sweep | One `ControlPacket` per 100 ms (10 pkts/sec). Cycles through 4 FA-18C controls. |
| `F` | Fast burst | One `ControlPacket` per 2 ms (~500 pkts/sec). |
| `X` | Extreme | Back-to-back with no delay — intentionally saturates the path. |
| `T` | Throughput | 1000 TEST_SEQ packets; prints RTT histogram on completion. |

### Diagnostic output (STM32 master PA9, 115200)

```
[HB]     node=1 flags=0x00 rx=823 ESR=0x0000   ← sub-node heartbeat (every 500 ms)
[EVT]    node=1 ctrl=0x0001 val=1               ← button press on sub-node PB1
[RTT]    seq=42 ~rtt=2ms                        ← round-trip echo result
[ERRS]   TEC=0 REC=0 flags=0x00                 ← CAN error counters (every 1 s)
[TXFULL] total=1                                ← master CAN TX mailbox was full
[OVF]    align total=1                          ← UART alignment loss on master
[DEAD]   node=2                                 ← no heartbeat for 3 s
```

`flags` in `[HB]` is the sub-node's status byte: bit 0 = BOFF, bit 1 = EPVF, bit 2 = TX drops (mailbox-full count > 0 since boot). ESR is `CAN1->ESR[31:16]` — upper 16 bits containing TEC[23:16] and REC[31:24].

### LED status (PC13, active-low, all three boards)

| Pattern | Meaning |
|---------|---------|
| 1 Hz blink | Normal |
| 5 Hz blink | CAN TEC > 0 (errors detected) |
| Solid on | Bus-off state |

On the **master**: fast blink means at least one sub-node has missed its heartbeat for more than 3 seconds.

---

## Experiment A — UART Byte-Stream Capture

**What this measures:** The volume and burst shape of the DCS-BIOS data stream from a live FA-18C session. Results size the STM32 UART RX buffer and estimate the maximum CAN frame rate the production system will need to sustain.

**What this does not test:** CAN bus behaviour. Run this experiment with the STM32 disconnected — the Arduino measures the byte stream whether or not anything is listening on Serial1.

### Wiring (Experiment A)

```
PC (DCS-BIOS USB) → Arduino USB (Serial, 250000)
Arduino Serial2 pin 16 (TX) → USB-TTL adapter 2 RX   [analytics console]
Arduino Serial1 — leave unconnected
```

The STM32 is not involved. Serial1 transmits into the air; the firmware doesn't care. Connecting the STM32 during this step adds `[OVF]` noise from the misframed relay without adding useful information — defer that to Experiment B.

### Procedure

1. Wire as above. Open a terminal on **USB-TTL adapter 2** (115200) — this is the analytics console.
2. Load DCS and spawn a FA-18C. Wait for the aircraft to finish initialising.
3. Send `D` on Serial2 to reset capture statistics (DCS capture mode starts automatically at boot, but `D` gives a clean baseline).
4. Fly for 5 minutes with realistic cockpit activity — switch sweeps, system runups, a circuit. The goal is representative traffic, not minimum or maximum.
5. After 5 minutes the firmware prints a summary on Serial2, resets stats, and continues capturing. Send `I` to stop.

**Record:**
- Peak 100 ms burst (bytes) → minimum STM32 UART RX buffer size
- Average byte rate (B/s) → UART load metric; do not treat this as a CAN frame rate estimate. The production RP2040 translator will emit selected `ControlPacket`s, not one frame per 4 raw bytes, so actual CAN traffic will be significantly lower.
- Min/max inter-frame gap (ms) → confirms burst vs. continuous stream character

**FA-18C vs A-4E-C note:** FA-18C has more total controls (~285 vs ~198) but A-4E-C has roughly 6× more float outputs (89 vs ~15). Float outputs update continuously and drive higher sustained byte rates — the A-4E-C stream is the harder production case. Run this experiment against the A-4E-C when the mod is installed in Saved Games rather than the model viewer work tree.

---

## Experiment B — Synthetic CAN Bus Stress

**What this measures:** CAN bus topology correctness, round-trip latency, and the saturation limits of the UART → CAN path. No DCS-BIOS or PC connection is required.

**What this does not test:** Real DCS-BIOS semantic content. The Arduino generates synthetic `ControlPacket` structs. This is not representative of DCS-BIOS data volume or framing, but it is the correct way to stress the CAN bus independently.

**Note on master diagnostic frames:** The master sends `0xAA`-prefixed binary frames (RTT results, heartbeat status, error reports) back to the Arduino over the same UART2 link. The Arduino sketch handles these. If you connect an oscilloscope or logic analyser to PA2/PA3 you will see both directions simultaneously.

### Wiring (Experiment B)

```
Arduino Serial1 pin 18 (TX) → 1 kΩ → STM32 Master PA3 ← 2 kΩ → GND
Arduino Serial1 pin 19 (RX) ← STM32 Master PA2
Arduino Serial2 pin 16 (TX) → USB-TTL adapter 2 RX
USB-TTL adapter 1 RX ← STM32 Master PA9
GND ─── GND (all boards)

CAN BUS (daisy-chain, 24 AWG twisted pair + GND wire):
  [120 Ω] ── Master SN65HVD230 ──── Sub-1 SN65HVD230 ──── Sub-2 SN65HVD230 ── [120 Ω]
```

120 Ω termination at the two physical endpoints only (Master and Sub-2). Sub-1 is intermediate — no resistor. Run a GND wire alongside CANH/CANL; the SN65HVD230 references signal levels to GND.

### Steps

**Step 1 — Bit timing verification (master only, no bus)**

Before connecting any transceiver, confirm the crystal, PLL, and CAN bit timing are correct.

In `CAN_Test_Master/src/main.cpp`, temporarily change:
```cpp
hcan.Init.Mode = CAN_MODE_NORMAL;
```
to:
```cpp
hcan.Init.Mode = CAN_MODE_LOOPBACK;
```

Rebuild and flash. In loopback mode the CAN controller's TX feeds its own RX internally — no transceiver or bus is needed. Send `S` on Serial2. Watch the diagnostic tap for 30 seconds.

Pass: no `[ERRS]` lines with non-zero TEC or REC. Any non-zero TEC or REC means the bit timing is wrong — check the crystal and PLL configuration.

Note: the master LED will **fast-blink** throughout this step because both sub-nodes are absent and the watchdog considers them dead. Fast-blink here is expected and does not indicate a failure.

Loopback does not exercise sub-node echo. The `T` throughput test will time out because the master does not parse `ID_TEST_SEQ` on receive. Use `S` or `F` for traffic generation in this step only.

Restore `CAN_MODE_NORMAL`, rebuild, and flash before proceeding.

**Step 2 — Two-node bus**

Wire Master and Sub-1 with 120 Ω at each end. Flash Sub-1 (PA0 strapped to 3.3 V → `node_id = 1`). Send `S` on Serial2.

Note: the master LED will fast-blink in this step because Sub-2 is absent and the watchdog considers it dead. This is expected until Step 3.

Pass criteria after 5 minutes:
- `[HB] node=1` appears every ~500 ms
- `[ERRS]` TEC = 0, REC = 0, flags = 0x00
- Sub-node heartbeat `flags` byte = 0x00 (no BOFF, EPVF, or TX drops)

**Step 3 — Three-node bus**

Add Sub-2. Move the far-end 120 Ω to Sub-2 (remove the one at Sub-1). Flash Sub-2 (PA0 floating → `node_id = 2`). Both `[HB] node=1` and `[HB] node=2` must appear within 1 second of each other.

**Step 4 — INPUT_EVENT round-trip**

Press the button wired to Sub-1 PB1. Diagnostic tap must print `[EVT] node=1 ctrl=0x0001 val=1` within 5 ms of the physical press. Release — `val=0`. Both edges must appear. Note: PB1 has no debounce; a single press may produce multiple edges on a noisy switch. Count events over time, not per press.

**Step 5 — Fast burst**

Send `F` (~500 pkts/sec, 2 ms interval). Run for 30 seconds.

Pass criteria: zero `[TXFULL]`, zero `[OVF]`, heartbeat `rx` counter increments proportionally, sub-node `flags` = 0x00.

**Step 6 — Extreme burst (bottleneck hunt)**

Send `X` (back-to-back, no delay). Run for 20 seconds. Record the **first** symptom to appear:

| First symptom | Bottleneck identified |
|---------------|-----------------------|
| `[OVF]` | STM32 UART RX buffer too small |
| `[TXFULL]` | CAN TX mailbox saturates before main loop clears it |
| `[ERRS]` TEC > 0 | Bus errors under load (termination, GND, or timing issue) |
| Sub-node `flags` bit 2 set | Sub-node TX mailbox full (echo/heartbeat drops) |
| Heartbeat `rx` grows slower than injected count | CAN frame loss |

**Step 7 — Throughput test**

Send `T` (1000 TEST_SEQ packets). The Arduino prints an RTT histogram on Serial2 when complete. Record the full distribution.

Pass: no entries in the >20 ms bucket under normal two-node conditions. If the tail bucket fills at fast burst rate, check interrupt latency and CAN NVIC priority.

---

## Experiment C — DCS-BIOS Bridge Validation

**Status: blocked until RP2040 Bridge firmware implements a DCS-BIOS → ControlPacket translator.**

**What this will test:** That real DCS-BIOS cockpit state (switch positions, analog values) propagates from the PC through the RP2040 → UART → STM32 master → CAN bus → sub-nodes. This is the production data path.

**Why it is blocked:** The RP2040 Bridge firmware (`Firmware/HID_Controllers/DCS_BIOS_Bridge/`) currently relays raw DCS-BIOS bytes transparently. The STM32 master expects 4-byte `ControlPacket` structs (`{uint16_t controlId; uint16_t value;}`). Without a translator the master receives misframed data and emits `[OVF]` alignment errors — CAN nodes do not receive any meaningful state.

**What needs to be built — two independent halves:**

1. **DCS → cockpit** (downstream): DCS-BIOS output callbacks on the RP2040 intercept specific control state values (e.g. `MASTER_ARM_SW`, `HUD_VIDEO_BRT`) and serialise them as `ControlPacket` structs on `Serial1` to the STM32 master, which broadcasts them over CAN to sub-nodes. See `Docs/claude/architecture.md` — "DCS-BIOS Integration Constraint" section for the packet format and example code.

2. **Cockpit → DCS** (upstream): When a sub-node detects a button press it transmits an INPUT_EVENT on CAN. The STM32 master already forwards these to the RP2040 as `ControlPacket` structs over UART2. The RP2040 must parse incoming `ControlPacket`s on `Serial1` and translate them to `sendDcsBiosMessage()` calls. Without this half, physical cockpit inputs have no effect in the sim. See `Docs/claude/architecture.md` — "DCS-BIOS Integration Constraint" section for the translation pattern.

**Wiring once unblocked:**

```
RP2040 GP0 (TX) → 1 kΩ → STM32 Master PA3 ← 2 kΩ → GND
RP2040 GP1 (RX) ← STM32 Master PA2
RP2040 USB → PC (DCS-BIOS COM port)
USB-TTL adapter 1 RX ← STM32 Master PA9
```

The Arduino is **not connected** in Experiment C. The UART2 link belongs entirely to the RP2040. Synthetic injection (modes S/F/X/T) is not possible during this experiment. Stress the path by driving cockpit activity in DCS — switch sweeps, axis deflections — and monitor TEC/REC and TXFULL on the diagnostic tap.

**Note on binary diagnostic frames:** The master sends `0xAA`-prefixed binary frames (RTT, heartbeat, error reports) back over UART2 to the RP2040. The current RP2040 Bridge firmware treats these as noise. This is acceptable because PA9 is the primary monitoring channel. If the RP2040 bridge is extended later, add a handler that checks for the `0xAA` magic byte and routes those frames to a separate output rather than forwarding them to the DCS-BIOS serial port.

**Success criteria (when unblocked):**
- Sub-node heartbeat `rx` count increments during a live DCS session (confirming CONTROL_BROADCAST frames reach CAN nodes)
- Zero CAN errors over a 5-minute run
- No `[OVF]` on the master with concurrent DCS-BIOS traffic

---

## Interpreting Results

| Symptom | Root cause | Mitigation |
|---------|-----------|------------|
| `[OVF]` at moderate injection rate | STM32 UART RX buffer too small | Increase `SERIAL_RX_BUFFER_SIZE` beyond 256; add DMA UART RX |
| `[TXFULL]` before extreme burst | Main loop blocks before mailbox clears | Add a software CAN TX queue (ring buffer) |
| Sub-node `flags` bit 2 set | Sub-node mailbox full during echo/heartbeat | Check CAN bus load; sub-node TX backlog clears automatically once bus quiets |
| TEC climbing only with DCS active | UART ISR delays CAN inter-frame timing | Raise CAN NVIC priority above UART DMA |
| RTT tail >20 ms at fast burst | CAN arbitration delay under contention | Normal at extreme load; investigate if it appears at `F` mode rate |
| No heartbeat from sub-node | Wiring, termination, or transceiver power | Verify SN65HVD230 VCC = 3.3 V; confirm 120 Ω at endpoints only; confirm GND wire alongside bus |
