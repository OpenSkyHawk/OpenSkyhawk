# CAN Bus Integration Test

This test validates the CAN bus architecture before any production PCBs are built. The goal is to find bottlenecks and failure modes on the bench — the same way the STM32 USB CDC crash was discovered during the DCS-BIOS debug session (see `Docs/claude/dcsbios-stm32-debug.md`).

## What We're Testing

The production architecture routes DCS-BIOS state from the PC through an RP2040 bridge, down a UART link to an STM32 CAN master node, and then out over CAN bus to all cockpit panel nodes. The specific things we need to stress before committing to PCBs:

1. **CAN bus topology** — do messages actually reach sub-nodes, and do heartbeats come back?
2. **UART buffer behaviour** — does the STM32 drop bytes under real DCS-BIOS load?
3. **CAN TX mailbox saturation** — does the master fall behind when flooded with packets?
4. **CAN error counters under load** — do TEC/REC climb when UART and CAN run concurrently?
5. **Round-trip latency** — what does the full Arduino → CAN → sub-node → echo → Arduino path actually cost?

The FA-18C is used as a test aircraft because it is complex enough to generate realistic DCS-BIOS traffic. The A-4E-C has 6× more float outputs than the FA-18C and is likely the harder production case, but the mod must be in Saved Games (not the model viewer work tree) to fly it — defer that run until development is paused.

## Hardware Required

| Qty | Part | Notes |
|-----|------|-------|
| 1 | Arduino Mega 2560 | UART injector + DCS-BIOS relay + analytics |
| 3 | STM32F103CBT6 Blue Pill | Must have external 8 MHz crystal |
| 3 | SN65HVD230 CAN transceiver | 3.3 V supply — **not** 5 V |
| 2 | 120 Ω resistors | CAN bus termination |
| 2 | USB-TTL adapters | One for STM32 master PA9 (diagnostic); one for Arduino Serial2 (commands + analytics) |
| — | 24 AWG twisted pair | CANH/CANL + a GND wire run alongside |
| — | 1 kΩ + 2 kΩ resistors | Voltage divider on Arduino TX → STM32 PA3 |

## Wiring

### Phase 1 (Arduino only, no PC/DCS)

```
ARDUINO MEGA 2560
  Serial1 pin 18 (TX) → 1kΩ → STM32 Master PA3 ← 2kΩ → GND
  Serial1 pin 19 (RX) ←────────────────────────── STM32 Master PA2
  Serial2 pin 16 (TX) → USB-TTL adapter #2 RX  (command input + analytics)
  GND ──────────────────────────────────────────── GND

USB-TTL ADAPTER #1 (keep connected in both phases)
  RX ←── STM32 Master PA9 (UART1 TX, 115200)
  GND ── GND

CAN BUS (24 AWG twisted pair + GND wire)
  STM32 Master ──[120Ω]── SN65HVD230 ─── CANH/CANL ─── SN65HVD230 ── STM32 Sub-1
                                                     │
                                              SN65HVD230 ── STM32 Sub-2 ──[120Ω]
```

Termination: 120 Ω at the Master end and at Sub-2 (the far end). Sub-1 is intermediate — no resistor. Run a GND wire alongside CANH/CANL; the SN65HVD230 needs common ground between all boards or the common-mode headroom is eaten up.

**Node ID strap:** same binary is flashed to both sub-nodes. At startup the firmware reads PA0:
- Sub-1: solder a wire from PA0 to the 3.3 V rail → reads HIGH → node_id = 1
- Sub-2: leave PA0 unconnected (firmware enables internal pull-down) → reads LOW → node_id = 2

### Phase 2 (full path, PC + RP2040 + DCS)

Disconnect the Arduino's Serial1 from the master. Plug in the RP2040 Bridge:

```
RP2040 GP0 (TX) → [1kΩ/2kΩ divider] → STM32 Master PA3
RP2040 GP1 (RX) ←──────────────────── STM32 Master PA2
RP2040 USB → PC (DCS-BIOS COM port)
```

The USB-TTL adapter on PA9 stays connected throughout — this is the diagnostic window in both phases and does not conflict with the DCS-BIOS COM port.

**Note on binary diagnostic frames:** In Phase 1 the master sends `0xAA`-prefixed diagnostic frames (RTT results, heartbeat status, error reports) back over the same UART2 link to the Arduino. The Arduino sketch handles these. In Phase 2 the RP2040 will receive these same frames — the existing RP2040 Bridge firmware does not parse them and will treat them as noise. This is acceptable for Phase 2 testing because the diagnostic tap on PA9 is the primary monitoring channel; the UART2 diagnostic frames are supplemental. If you extend the bridge firmware later, add a handler for the `0xAA` magic byte prefix.

## Firmware

| Project | Target | Role |
|---------|--------|------|
| `CAN_Test_Arduino/` | Mega 2560 | Packet injector, DCS-BIOS relay, analytics |
| `CAN_Test_Master/` | Blue Pill | UART → CAN bridge, error monitor, node watchdog |
| `CAN_Test_SubNode/` | Blue Pill ×2 | CAN sub-node, heartbeat, echo, button event |

Flash with `pio run -t upload` inside each project directory. The Blue Pill projects include the Chinese-clone JTAG ID override (`0x2ba01477`) in `platformio.ini` — no extra flags needed.

### Arduino commands (send a single character over **Serial2** USB-TTL adapter, 115200)

> **Serial ports at a glance:**
> - Arduino USB (`Serial`, 250000) — DCS-BIOS input only. Do not open this in a monitor that sends keystrokes; DCS-BIOS owns it in Phase 0/2.
> - Arduino Serial2 USB-TTL adapter (115200) — commands in, analytics out. This is the operator console in all phases.
> - STM32 Master PA9 USB-TTL adapter (115200) — CAN diagnostic output only.

| Key | Mode | Description |
|-----|------|-------------|
| `D` | DCS capture | Serial is already at 250000 from boot. `D` resets capture statistics and confirms relay mode is active. Analytics print to Serial2 every second. |
| `I` | Idle | Stop all injection |
| `S` | Slow sweep | One ControlPacket per 100 ms (10 pkts/sec). Cycles through 4 FA-18C controls. |
| `F` | Fast burst | One packet per 2 ms (~500 pkts/sec) |
| `X` | Extreme | Back-to-back, no delay — intentionally saturates the path |
| `T` | Throughput | Sends 1000 TEST_SEQ packets; prints RTT histogram when done |

**Serial2** receives all analytics output regardless of mode — it remains readable even in Phase 2 when Serial is owned by DCS-BIOS.

### Diagnostic output (USB-TTL on STM32 Master PA9, 115200)

```
[HB]     node=1 flags=0x00 rx=823 ESR=0x0000   ← sub-node heartbeat
[EVT]    node=1 ctrl=0x0001 val=1               ← button press on sub-node PB1
[RTT]    seq=42 ~rtt=2ms                        ← round-trip echo result
[ERRS]   TEC=0 REC=0 flags=0x00                 ← CAN error counters (every 1s)
[TXFULL] total=1                                ← CAN TX mailbox was full
[OVF]    align total=1                          ← UART alignment loss (byte dropped)
[DEAD]   node=2                                 ← no heartbeat for 3 seconds
```

### LED status (PC13, active-low, all three boards)

| Pattern | Meaning |
|---------|---------|
| 1 Hz blink | Normal |
| 5 Hz blink | CAN TEC > 0 (errors detected) |
| Solid on | Bus-off state |

On the master: fast blink means at least one sub-node is not heartbeating.

## Test Procedure

### Phase 0 — DCS-BIOS Byte Stream Capture

**Purpose:** Characterise the real DCS-BIOS traffic volume before stressing the CAN bus. Results determine the right UART RX buffer size on the STM32 master.

**Important:** Phase 0 is a byte-load measurement, not a semantic bridge. The Arduino relays raw DCS-BIOS bytes to the STM32 master, which will try to interpret them as 4-byte ControlPackets and emit `[OVF]` alignment errors. This is expected — the OVF count is a side-effect of the mismatched framing, not a real loss event. Ignore `[OVF]` on the diagnostic tap during Phase 0; focus on the byte-rate analytics printed on Serial2.

1. Wire Arduino Mega USB to PC. Wire Arduino Serial1 (pins 18/19) to STM32 Master PA2/PA3. Wire Arduino Serial2 (pins 16/17) to USB-TTL adapter #2.
2. Open two serial monitors: one on the **Serial2 USB-TTL adapter** (115200) for commands and analytics; one on the **STM32 Master PA9 USB-TTL adapter** (115200) for CAN diagnostics. Do not open the Arduino USB port in a monitor — DCS-BIOS owns it.
3. Load DCS, start a FA-18C flight, wait for aircraft to finish spawning.
4. Send `D` in the Serial2 terminal to confirm DCS capture mode is active (it starts automatically at boot, but `D` resets the stats for a clean run).
5. Fly normally for 5 minutes — flick switches, run up systems, fly a circuit. The more cockpit activity the better.
6. After 5 minutes the firmware prints the summary on Serial2, resets stats, and continues DCS capture. Send `I` to stop relaying if desired.

**Record from the summary:**
- Peak 100 ms burst (bytes) → this is the minimum STM32 RX buffer size needed
- Average byte rate (B/s) → divide by 4 to estimate CAN frames per second needed
- Min/max inter-frame gap (ms) → confirms whether DCS sends bursts or a continuous stream

### Phase 1 — CAN Topology (no PC/DCS required)

**Step 1 — CAN loopback (master only)**

This step verifies crystal frequency, PLL lock, and HAL CAN bit timing before any bus hardware is involved.

In `CAN_Test_Master/src/main.cpp`, change:
```cpp
hcan.Init.Mode = CAN_MODE_NORMAL;
```
to:
```cpp
hcan.Init.Mode = CAN_MODE_LOOPBACK;
```

Reflash. In loopback mode the CAN controller's TX is internally connected to its RX — no transceiver or bus wiring is needed. Send `S` from the Serial2 terminal. Watch the STM32 diagnostic tap: if `[ERRS]` lines appear with non-zero TEC/REC, the bit timing is wrong. If the master LED blinks at 1 Hz with no errors after 30 seconds, the timing is correct.

Note: loopback does not exercise sub-node echo. The master transmits frames and receives them back internally, but `processCan()` on the master does not handle `ID_TEST_SEQ` on receive, so `T` mode will time out without RTT results. Use `S` or `F` to generate traffic for error observation only.

Restore `CAN_MODE_NORMAL` and reflash before the next step.

**Step 2 — Two-node bus**

Wire Master and Sub-1 with 120 Ω at each end. Flash Sub-1 (with PA0 strapped to 3.3 V). Send `S` from the Serial2 terminal. Watch the diagnostic tap:
- `[HB] node=1` should appear every ~500 ms
- `[ERRS]` TEC and REC should remain 0

Run for 5 minutes. Pass = zero errors, heartbeats continuous.

**Step 3 — Three-node bus**

Add Sub-2. Move the far-end termination resistor to Sub-2 (Sub-1 becomes intermediate — remove its resistor). Flash Sub-2 (PA0 floating). Both `[HB] node=1` and `[HB] node=2` should appear.

**Step 4 — INPUT_EVENT round-trip**

Press the button wired to Sub-1 PB1. The diagnostic tap should immediately print `[EVT] node=1 ctrl=0x0001 val=1`. Release — `val=0`. Both edges should appear within 5 ms of the physical button press.

**Step 5 — Fast burst**

Send `F` (~500 pkts/sec, 2 ms interval). Run for 30 seconds. Check diagnostic tap for any `[TXFULL]` or `[OVF]`. Heartbeat `rx` counters should increment proportionally. Pass = zero errors.

**Step 6 — Extreme burst (bottleneck hunt)**

Send `X`. Let it run for 20 seconds. Watch for:
- `[TXFULL]` — CAN TX mailbox saturated (STM32 CAN tx queue is the limit)
- `[OVF]` — UART byte loss (STM32 RX buffer overflowed)
- `[ERRS]` with TEC > 0 — bus errors under load
- Heartbeat `rx` count growing slower than injected count — CAN frame loss

**Record the first symptom that appears and at what point in the run.** This is the bottleneck.

**Step 7 — Throughput test**

Send `T`. The Arduino prints a RTT histogram after 1000 packets. Record the distribution — if the >20 ms bucket has any entries under normal two-node conditions something is wrong with interrupt latency.

### Phase 2 — Full DCS-BIOS Path

1. Disconnect Arduino Serial1 from master. Connect RP2040 Bridge to master PA2/PA3 and to PC USB. DCS-BIOS should connect automatically (the RP2040 enumerates as a USB CDC serial device).
2. The USB-TTL on master PA9 stays connected — this remains the diagnostic window.
3. Load DCS, spawn FA-18C. Confirm `[HB]` messages continue on the diagnostic tap while DCS is running — this verifies DCS-BIOS downstream data (PC → RP2040 → STM32 → CAN) is flowing and sub-nodes are receiving.
4. Verify sub-node `rx` count in heartbeats increases during a live DCS session.
5. With DCS live, drive cockpit activity — flip switches rapidly, slew axes — to maximise DCS-BIOS output rate. Watch for `[ERRS]` TEC/REC climbing or `[TXFULL]` appearing. This is the Phase 2 equivalent of the extreme burst: real DCS-BIOS traffic rather than synthetic injection, since the Arduino injector is disconnected in this phase.

**Success criteria:**
- Sub-node `rx` count grows during live DCS session
- Zero CAN errors over a 5-minute concurrent run
- RTT histogram (from Phase 1 Step 7): no entries in >20 ms bucket at fast burst rate
- No `[OVF]` (UART overruns) on master with concurrent DCS traffic

## Interpreting Results and Next Steps

| Symptom | Root cause | Mitigation |
|---------|-----------|------------|
| `[OVF]` before 3000 pkts/sec | STM32 UART RX buffer too small | Increase `SERIAL_RX_BUFFER_SIZE` beyond 256; add DMA UART RX |
| `[TXFULL]` at moderate rate | Main loop blocks before mailbox clears | Add a software CAN TX queue (ring buffer) |
| TEC climbing only in Phase 2 | UART ISR delays CAN inter-frame timing | Raise CAN NVIC priority above UART |
| RTT tail >20 ms | CAN arbitration delay under contention | Normal at extreme load; check if it appears at fast burst rate |
| No heartbeat from sub-node | Wiring, termination, or transceiver power | Verify SN65HVD230 VCC = 3.3 V; check 120 Ω placement; confirm GND wire |

The results from Phase 0 (byte rate) and Steps 5/6 (burst behaviour) feed directly into the production CAN master firmware design — buffer sizes, DMA vs. polling decisions, and CAN TX queue depth all come from measured numbers rather than guesses.
