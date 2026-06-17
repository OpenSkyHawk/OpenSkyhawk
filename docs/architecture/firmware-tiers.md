# Firmware Tiers

OpenSkyhawk firmware runs across three tiers. Each owns a clear slice of the job and hands
off to the next over a defined link. The contracts between tiers — what each tier does
*not* do — matter as much as what it does. Get them wrong and you'll duplicate logic or
break the byte-relay guarantees the architecture depends on.

```
PC (DCS + DCS-BIOS)
  │  USB — single connection, composite CDC + HID
  ▼
SimGateway   (RP2040)
  │  UART @ 250 kbps
  ▼
PanelBridge  (STM32F103C8, CAN master)
  │  CAN bus @ 500 kbps
  ▼
PanelGroup   (STM32F103C8, one per panel group)
  │  I²C over JST-XH
  ▼
Breakout boards (MCP23017, ADS1115)
```

## The three tiers at a glance

| | SimGateway | PanelBridge | PanelGroup |
|---|---|---|---|
| **MCU** | RP2040 | STM32F103C8 | STM32F103C8 |
| **Library** | `SimGateway.h` | `PanelBridge.h` | `OpenSkyhawk.h` |
| **Role** | USB bridge + HID | CAN master + DCS-BIOS | Panel I/O |
| **Upstream** | USB to PC | UART to SimGateway | CAN to PanelBridge |
| **Downstream** | UART to PanelBridge | CAN to PanelGroup nodes | I²C to breakouts |
| **Count** | 1 per cockpit | 1 per cockpit | 1 per panel group |

Three more libraries are shared rather than owned by a single tier: **`CANProtocol`** (packet
structs, CAN IDs, the `controlId` namespace), **`STM32Board`** (CAN/UART/status-LED hardware
init for both STM32 tiers), and **`HIDControls`** (the HID `controlId` allocations, shared
between SimGateway and CANProtocol).

!!! note "STM32F103 variant: STM32F103C8 by default on every board"
    Both STM32 tiers use the STM32F103 family, and both default to the **STM32F103C8** (64 KB
    flash). **PanelBridge** carries the full DCS-BIOS input map on top of the DCS-BIOS library,
    yet still compiles to ~26 KB flash (~40% of the C8's 64 KB), so it fits the STM32F103C8.
    PanelGroup nodes only hold the `DCSIN_*` constants they use, so they fit easily too. The
    **STM32F103CB** (128 KB) is a drop-in fallback on the identical LQFP48 footprint, available
    if a future build ever exceeds 64 KB — no board currently needs it.

## SimGateway (RP2040)

The single USB device the PC sees. A composite device: USB CDC for the DCS-BIOS stream, USB
HID for flight-control axes and buttons. The CDC interface advertises its own name —
`A-4E Skyhawk DCS-BIOS` (`iInterface`) — so the serial port is self-identifying, not just a
generic VID/PID + CDC class. It relays the DCS-BIOS byte stream transparently to PanelBridge
over UART, and intercepts HID frames coming back up to drive the joystick report.

**What it does *not* do:**

- It does **not** run the DCS-BIOS library. It never parses or interprets the DCS-BIOS stream
  — every byte is relayed verbatim.
- It has **no knowledge** of DCS-BIOS control names, addresses, or the input map.
- It does **not** buffer or delay DCS-BIOS bytes — relay is immediate, so PanelBridge sees the
  stream with USB-CDC latency only.

See [SimGateway](sim-gateway.md) for the full breakdown.

## PanelBridge (STM32F103)

The CAN master, and the only tier that runs the DCS-BIOS library. It receives the DCS-BIOS
stream from SimGateway over UART, fires the DCS-BIOS export listener, and broadcasts every
DCS output onto the CAN bus as `CTRL_BCAST` frames. In the other direction it collects input
events (`EVT_n`) from the nodes and routes each one: DCS-BIOS inputs go out as ASCII commands
over UART; HID inputs are wrapped in HID frames for SimGateway. It also tracks which PanelGroup
nodes are alive (from their heartbeats) and — behind a build flag — reports that roster + health
to the host over DCS-BIOS itself (`_NODE_STATUS` messages; request address `0x86FE`), so the client
can show connected panels without any SimGateway change
([#86](https://github.com/OpenSkyHawk/OpenSkyhawk/issues/86)).

**What it does *not* do:**

- It does **not** touch USB at runtime. (USB and CAN share pins PA11/PA12 on the STM32 — they
  can't both be active. That's the whole reason SimGateway exists.)
- It does **not** track which node sent an input event — PanelBridge has no per-node input
  state. Multiple controls may map to the same DCS command.
- It does **not** drive panel hardware decisions; nodes own their own I/O.

`NODE_ID` is always **0** for PanelBridge — reserved, and never transmitted as a heartbeat on
the bus.

## PanelGroup (STM32F103)

One per panel group. Each node drives its panel hardware directly — GPIO, or MCP23017
(digital I/O) and ADS1115 (analog) breakouts over I²C — and carries a unique `NODE_ID` (1–63)
baked in at compile time. It receives `CTRL_BCAST` frames and dispatches them to registered
output objects (LEDs, gauges); it polls input objects and fires `EVT_n` frames back to
PanelBridge.

**What it does *not* do:**

- It does **not** decide DCS-BIOS vs HID routing. A node just fires a `controlId` and a value;
  the routing happens upstream (PanelBridge for DCS-BIOS, SimGateway for HID). See
  [DCS-BIOS vs HID](dcsbios-vs-hid.md).
- It does **not** run the DCS-BIOS library or know any DCS command strings — it works in
  compact `controlId` / value terms only.
- It does **not** talk to other nodes. All traffic is node ↔ PanelBridge.

## Why the split

Each boundary exists for a concrete reason — USB reliability, peripheral conflicts, keeping
nodes dumb and routing centralised. Those reasons, with the failure data behind them, are in
[Design Decisions](design-decisions.md).
