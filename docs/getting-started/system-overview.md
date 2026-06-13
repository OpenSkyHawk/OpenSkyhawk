# System Overview

OpenSkyhawk uses a three-tier firmware architecture to connect physical cockpit panels to
DCS. Each tier has a distinct role: the top tier handles USB and HID, the middle tier
manages the CAN bus cluster, and the bottom tier drives panel hardware directly.

## Architecture Diagram

<svg viewBox="0 0 760 520" xmlns="http://www.w3.org/2000/svg" role="img" style="max-width:100%;height:auto;display:block;margin:1.5rem auto;">
  <title>OpenSkyhawk System Architecture</title>
  <desc>Three-tier firmware architecture: SimGateway (RP2040) connects via USB to PC and UART to PanelBridge (STM32 CAN master), which distributes to PanelGroup nodes over CAN bus.</desc>

  <rect width="760" height="520" fill="#1a1a2e" rx="10"/>

  <text x="380" y="32" text-anchor="middle" fill="#8888aa" font-size="12" font-family="system-ui,sans-serif" font-weight="500" letter-spacing="1">SYSTEM ARCHITECTURE</text>

  <!-- PC -->
  <rect x="30" y="55" width="150" height="80" rx="8" fill="#252540" stroke="#444466" stroke-width="1.5"/>
  <text x="105" y="83" text-anchor="middle" fill="#e0e0ff" font-family="system-ui,sans-serif" font-weight="600" font-size="14">PC</text>
  <text x="105" y="101" text-anchor="middle" fill="#8888aa" font-family="system-ui,sans-serif" font-size="11">DCS-BIOS</text>
  <text x="105" y="117" text-anchor="middle" fill="#8888aa" font-family="system-ui,sans-serif" font-size="11">+ HID Input</text>

  <!-- SimGateway -->
  <rect x="270" y="44" width="180" height="102" rx="8" fill="#1a2a1a" stroke="#3a8a3a" stroke-width="2"/>
  <text x="360" y="68" text-anchor="middle" fill="#7dbb7d" font-family="system-ui,sans-serif" font-weight="700" font-size="13">SimGateway</text>
  <text x="360" y="86" text-anchor="middle" fill="#5a9a5a" font-family="system-ui,sans-serif" font-size="11">RP2040</text>
  <rect x="281" y="94" width="72" height="38" rx="4" fill="#152515" stroke="#2a5a2a" stroke-width="1"/>
  <text x="317" y="108" text-anchor="middle" fill="#5a9a5a" font-family="system-ui,sans-serif" font-size="10">DCS-BIOS</text>
  <text x="317" y="123" text-anchor="middle" fill="#5a9a5a" font-family="system-ui,sans-serif" font-size="10">serial</text>
  <rect x="367" y="94" width="72" height="38" rx="4" fill="#152515" stroke="#2a5a2a" stroke-width="1"/>
  <text x="403" y="108" text-anchor="middle" fill="#5a9a5a" font-family="system-ui,sans-serif" font-size="10">HID</text>
  <text x="403" y="123" text-anchor="middle" fill="#5a9a5a" font-family="system-ui,sans-serif" font-size="10">joystick</text>

  <!-- USB: PC <-> SimGateway -->
  <line x1="180" y1="95" x2="270" y2="95" stroke="#4a8cff" stroke-width="2" stroke-dasharray="6,3"/>
  <text x="225" y="88" text-anchor="middle" fill="#4a8cff" font-family="system-ui,sans-serif" font-size="11" font-weight="600">USB</text>
  <polygon points="268,90 276,95 268,100" fill="#4a8cff"/>
  <polygon points="182,90 174,95 182,100" fill="#4a8cff"/>

  <!-- PanelBridge -->
  <rect x="290" y="215" width="140" height="80" rx="8" fill="#1a1a2a" stroke="#4a4aaa" stroke-width="2"/>
  <text x="360" y="241" text-anchor="middle" fill="#9090ee" font-family="system-ui,sans-serif" font-weight="700" font-size="13">PanelBridge</text>
  <text x="360" y="259" text-anchor="middle" fill="#6060aa" font-family="system-ui,sans-serif" font-size="11">STM32 — NODE 0</text>
  <text x="360" y="277" text-anchor="middle" fill="#6060aa" font-family="system-ui,sans-serif" font-size="11">CAN master</text>

  <!-- UART: SimGateway <-> PanelBridge -->
  <line x1="360" y1="146" x2="360" y2="215" stroke="#ffaa22" stroke-width="2"/>
  <text x="382" y="183" fill="#ffaa22" font-family="system-ui,sans-serif" font-size="11" font-weight="600">UART</text>
  <text x="382" y="197" fill="#cc8811" font-family="system-ui,sans-serif" font-size="10">250 kbps</text>
  <polygon points="355,213 360,221 365,213" fill="#ffaa22"/>
  <polygon points="355,148 360,140 365,148" fill="#ffaa22"/>

  <!-- CAN bus trunk -->
  <line x1="100" y1="380" x2="660" y2="380" stroke="#cc4444" stroke-width="3"/>
  <text x="380" y="372" text-anchor="middle" fill="#cc4444" font-family="system-ui,sans-serif" font-size="12" font-weight="700">CAN BUS — 500 kbps</text>

  <!-- 120R terminators -->
  <rect x="52" y="370" width="48" height="20" rx="4" fill="#2a1515" stroke="#cc4444" stroke-width="1.5"/>
  <text x="76" y="383" text-anchor="middle" fill="#cc4444" font-family="system-ui,sans-serif" font-size="10" font-weight="600">120 Ω</text>
  <rect x="660" y="370" width="48" height="20" rx="4" fill="#2a1515" stroke="#cc4444" stroke-width="1.5"/>
  <text x="684" y="383" text-anchor="middle" fill="#cc4444" font-family="system-ui,sans-serif" font-size="10" font-weight="600">120 Ω</text>

  <!-- PanelBridge drop to CAN -->
  <line x1="360" y1="295" x2="360" y2="380" stroke="#cc4444" stroke-width="2"/>
  <circle cx="360" cy="380" r="5" fill="#cc4444"/>

  <!-- PanelGroup Node 1 -->
  <rect x="155" y="420" width="130" height="80" rx="8" fill="#1a1a2a" stroke="#4a4aaa" stroke-width="1.5"/>
  <text x="220" y="446" text-anchor="middle" fill="#9090ee" font-family="system-ui,sans-serif" font-weight="600" font-size="13">PanelGroup</text>
  <text x="220" y="464" text-anchor="middle" fill="#6060aa" font-family="system-ui,sans-serif" font-size="11">STM32 — NODE 1</text>
  <text x="220" y="482" text-anchor="middle" fill="#6060aa" font-family="system-ui,sans-serif" font-size="11">Center Armament</text>

  <!-- PanelGroup Node 2 -->
  <rect x="315" y="420" width="130" height="80" rx="8" fill="#1a1a2a" stroke="#4a4aaa" stroke-width="1.5"/>
  <text x="380" y="446" text-anchor="middle" fill="#9090ee" font-family="system-ui,sans-serif" font-weight="600" font-size="13">PanelGroup</text>
  <text x="380" y="464" text-anchor="middle" fill="#6060aa" font-family="system-ui,sans-serif" font-size="11">STM32 — NODE 2</text>
  <text x="380" y="482" text-anchor="middle" fill="#505088" font-family="system-ui,sans-serif" font-size="11">Left ECM</text>

  <!-- PanelGroup Node N (future) -->
  <rect x="475" y="420" width="130" height="80" rx="8" fill="#1a1a2a" stroke="#444466" stroke-width="1.5" stroke-dasharray="5,3"/>
  <text x="540" y="446" text-anchor="middle" fill="#707098" font-family="system-ui,sans-serif" font-weight="600" font-size="13">PanelGroup</text>
  <text x="540" y="464" text-anchor="middle" fill="#505078" font-family="system-ui,sans-serif" font-size="11">STM32 — NODE N</text>
  <text x="540" y="482" text-anchor="middle" fill="#505078" font-family="system-ui,sans-serif" font-size="11">future panels…</text>

  <!-- Node drops to CAN -->
  <line x1="220" y1="380" x2="220" y2="420" stroke="#cc4444" stroke-width="2"/>
  <circle cx="220" cy="380" r="5" fill="#cc4444"/>
  <line x1="380" y1="380" x2="380" y2="420" stroke="#cc4444" stroke-width="2"/>
  <circle cx="380" cy="380" r="5" fill="#cc4444"/>
  <line x1="540" y1="380" x2="540" y2="420" stroke="#aa3333" stroke-width="1.5" stroke-dasharray="4,3"/>
  <circle cx="540" cy="380" r="4" fill="#aa3333"/>

  <!-- Legend -->
  <rect x="580" y="55" width="155" height="145" rx="6" fill="#252540" stroke="#444466" stroke-width="1"/>
  <text x="657" y="75" text-anchor="middle" fill="#8888aa" font-family="system-ui,sans-serif" font-size="11" font-weight="600">LEGEND</text>
  <line x1="598" y1="90" x2="630" y2="90" stroke="#4a8cff" stroke-width="2" stroke-dasharray="5,3"/>
  <text x="638" y="94" fill="#e0e0ff" font-family="system-ui,sans-serif" font-size="11">USB</text>
  <line x1="598" y1="110" x2="630" y2="110" stroke="#ffaa22" stroke-width="2"/>
  <text x="638" y="114" fill="#e0e0ff" font-family="system-ui,sans-serif" font-size="11">UART</text>
  <line x1="598" y1="130" x2="630" y2="130" stroke="#cc4444" stroke-width="2.5"/>
  <text x="638" y="134" fill="#e0e0ff" font-family="system-ui,sans-serif" font-size="11">CAN bus</text>
  <rect x="596" y="146" width="34" height="16" rx="3" fill="#2a1515" stroke="#cc4444" stroke-width="1.5"/>
  <text x="613" y="157" text-anchor="middle" fill="#cc4444" font-family="system-ui,sans-serif" font-size="9">120 Ω</text>
  <text x="638" y="158" fill="#e0e0ff" font-family="system-ui,sans-serif" font-size="11">termination</text>
  <rect x="596" y="168" width="34" height="16" rx="3" fill="#1a2a1a" stroke="#3a8a3a" stroke-width="1.5"/>
  <text x="613" y="179" text-anchor="middle" fill="#7dbb7d" font-family="system-ui,sans-serif" font-size="9">RP2040</text>
  <text x="638" y="179" fill="#e0e0ff" font-family="system-ui,sans-serif" font-size="11">SimGateway</text>
</svg>

## Firmware Tiers

### SimGateway (RP2040)

The top tier runs on a Raspberry Pi Pico or compatible RP2040 module. It has two
simultaneous roles over a single USB connection to the PC:

- **DCS-BIOS serial**: receives cockpit state updates from DCS (switch positions, gauge
  values) and forwards them over UART to PanelBridge as `ControlPacket` structs
- **HID joystick**: presents flight stick axes and buttons directly to the PC as a USB
  HID device — no DCS-BIOS involvement for flight controls

Cockpit inputs travel in the reverse direction: PanelGroup → CAN → PanelBridge → UART →
SimGateway → `sendDcsBiosMessage()` → DCS.

### PanelBridge (STM32, CAN master)

The middle tier runs on an STM32F103CBT6 and owns the CAN bus. It:

- Receives `ControlPacket` frames from SimGateway over UART and broadcasts them to all
  PanelGroup nodes over CAN (`CONTROL_BROADCAST`)
- Collects input events from PanelGroup nodes over CAN and forwards them upstream to
  SimGateway over UART

NODE_ID is always 0 (reserved for the master).

### PanelGroup (STM32, CAN sub-nodes)

The bottom tier. Each physical panel group has its own STM32F103CBT6 running the
PanelGroup library. Nodes receive `CONTROL_BROADCAST` frames and dispatch them to
registered output objects (LEDs, stepper gauges). Input objects (switches, pots) fire
input events back over CAN.

Each node has a unique `NODE_ID` (1–63) assigned at compile time via `platformio.ini`.
See [NODE_ID registry](../claude/firmware-node-ids.md) for assignments.

## CAN Bus

500 kbps, two-wire differential (CANH/CANL), daisy-chained across all boards. 120 Ω
termination resistors at the two physical endpoints only. The SN65HVD230 transceiver
(SOIC-8, 3.3 V) is used on every STM32 board.

**Key finding:** Blue Pill STM32 clones require SJW = 4TQ for reliable operation —
see [Design Decisions](../architecture/design-decisions.md#can-bus-timing--sjw-must-be-4tq-on-blue-pill-clones).
