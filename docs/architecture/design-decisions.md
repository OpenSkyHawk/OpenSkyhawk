# Design Decisions

The OpenSkyhawk architecture is the way it is for concrete reasons — most of them learned
the hard way on the bench. This page records those decisions with their failure modes and
the data behind them, so they don't get re-litigated. If you're about to argue "why not just
use the STM32's USB directly?" or "why is auto-retransmission off?", the answer is here.

Each decision lists the call that was made and the reasoning. Where a decision came from a
specific bench test, the empirical data is included.

---

## D1 — SimGateway exists because STM32 USB can't take the load

**Decision:** A dedicated RP2040 (the **SimGateway**) owns the USB connection to the PC and
acts as a pure byte relay. PanelBridge (STM32) runs the DCS-BIOS library but never touches
USB at runtime.

**The original approach:** Drive USB CDC directly from the STM32 — one MCU, DCS-BIOS library
and USB on the same chip. Simpler on paper.

**The failure mode:** Under sustained DCS-BIOS traffic the STM32 native USB CDC stack
**overflowed and crashed**. The port became unusable until a physical replug (`socat` exits
with "Permission denied"; the device falls off the bus). This is not a tuning problem — the
STM32F103 USB peripheral cannot reliably carry the full-state DCS-BIOS stream.

**Why RP2040 solves it:** The RP2040 has a native USB stack that handles the 250 kbps
DCS-BIOS stream without issue, and — critically — it has **no shared-peripheral conflict**.
On the STM32F103, USB and CAN share pins PA11/PA12 and cannot both be active. Moving USB to a
separate RP2040 frees the STM32 to dedicate its CAN peripheral to the bus, and keeps the
DCS-BIOS processing on a chip that never runs USB.

**The tradeoff:** One extra board and one UART hop between SimGateway and PanelBridge. That
hop is cheap (see [D2](#d2-uart-not-usb-cdc-for-the-simgateway-panelbridge-link)) and worth
it for a USB path that doesn't fall over.

!!! warning "Do not revert this"
    "Just use the STM32's USB" is the single most likely thing a new contributor will
    propose. It was tried. It crashes under real load. The SimGateway is not incidental
    complexity — it's the fix.

---

## D2 — UART, not USB CDC, for the SimGateway ↔ PanelBridge link

**Decision:** SimGateway and PanelBridge talk over a plain hardware **UART at 250 kbps**,
not over a second USB link.

**Reasoning:** The hardware UART showed **no throughput bottleneck** at any tested DCS-BIOS
load. The failure in [D1](#d1-simgateway-exists-because-stm32-usb-cant-take-the-load) was
specifically the STM32's USB peripheral, not its serial throughput. A UART is simple,
deterministic, has no enumeration or driver surface, and lets SimGateway stay a dumb byte
relay: any byte ≤ 0x7F is forwarded straight to USB CDC; HID frames are tagged with
high-bit magic bytes so they're structurally impossible to confuse with DCS-BIOS ASCII.

---

## D3 — CAN bus, not I²C, between boards

**Decision:** Inter-board communication uses a **CAN bus at 500 kbps**, not I²C. (I²C is used
only *within* a panel group, from a PanelGroup node out to its local breakout boards.)

**Reasoning:** CAN is built for exactly this job — a multi-drop network of nodes spread across
a physical structure:

- **Deterministic, prioritised arbitration** — frames don't collide and corrupt; the bus
  arbitrates by ID.
- **Hardware error detection and fault confinement** — CRC, ACK, and automatic error
  counters built into the controller, with graceful degradation to Error Passive rather than
  a hard failure.
- **Long, daisy-chained cable runs** — a differential pair tolerates the distances and noise
  of a full cockpit; I²C does not.
- **True multi-node topology** — nodes drop on and off the same two wires without a hub.

I²C has none of these properties over cockpit-scale wiring. It stays where it belongs:
short hops from an STM32 to an MCP23017 or ADS1115 on the same panel group.

---

## D4 — AutoRetransmission = DISABLE (this one will catch you out)

**Decision:** The STM32 bxCAN peripheral is configured with **`AutoRetransmission = DISABLE`**
and **`AutoBusOff = ENABLE`**. Both are correctness requirements, not preferences.

**The failure mode if you leave AutoRetransmission on (the default):** A single frame that
goes unacknowledged — one node briefly offline, one noisy moment — is retried forever by the
hardware. That one stuck frame **occupies a TX mailbox and never releases it**. With all
three TX mailboxes jammed this way, the transmit path deadlocks: the software TX queue can
never drain, and the node drives itself straight to bus-off. The whole node goes dark over a
single missed ACK.

**With AutoRetransmission disabled:** A failed send increments the Transmit Error Counter by
8 and the frame is dropped (the software queue applies its own bounded retry policy — see
[D6](#d6-controlid-routing-one-format-split-at-the-gateway) and the
[CAN Bus Protocol](can-bus.md) page). At TEC = 128 the node enters **Error Passive** and
limps along — CAN's intended graceful degradation — then self-heals when the fault clears.
No deadlock, no manual recovery.

**Why AutoBusOff = ENABLE matters too:** When a node does hit bus-off, hardware
auto-recovery brings it back in **~3 ms** (after 1408 recessive bits). With it disabled,
firmware would have to detect bus-off and restart the controller by hand.

Both settings were validated in the 2026-05-20 prototype soak and fault-injection tests.

| Setting | Required value | Consequence if wrong |
|---------|---------------|----------------------|
| `AutoRetransmission` | **DISABLE** | One unACKed frame jams all 3 TX mailboxes → immediate bus-off; TX queue never drains |
| `AutoBusOff` | **ENABLE** | Hardware recovers in ~3 ms; with DISABLE firmware must restart the controller manually |

---

## D5 — SJW = 4TQ, empirically determined

**Decision:** CAN bit timing is **Prescaler = 4, BS1 = 13TQ, BS2 = 4TQ, SJW = 4TQ** at
500 kbps. The synchronization jump width (SJW) of **4TQ** is the non-obvious part.

**The failure mode:** At SJW = 1TQ, a two-node bus produced **intermittent CRC errors**. The
Transmit Error Counter climbed on both nodes within minutes of starting traffic. The root
cause is crystal-to-crystal frequency tolerance between Blue Pill clones — small enough to be
in spec, large enough to break CAN sampling when the jump width is too narrow to absorb it.
Raising SJW to 4TQ widens the resynchronization window and absorbs the skew.

**The data (2026-05-20, two-node bench test, SJW = 4TQ):**

| Metric | Value |
|--------|-------|
| Soak duration | 21 minutes |
| Packets injected | 1257 @ 1 pkt/sec |
| Average RTT | 1 ms |
| Min / Max RTT | 1 ms / 1 ms |
| Packets lost | 0 |
| TEC / REC | 0 / 0 |
| Bus-off events | 0 |

The errors at SJW = 1TQ cleared **immediately** when SJW was raised to 4TQ, with no other
change made.

**Design rule:** All production `platformio.ini` files set SJW = 4TQ; the `CANProtocol`
library initialises CAN this way. Keep it permanently — it costs nothing even on production
PCBs with specified crystals.

!!! note "External crystal is mandatory"
    The STM32F103's internal RC oscillator (HSI, ±1%) is **not** accurate enough for 500 kbps
    CAN. Every MCU board must use an external **8 MHz crystal**, and firmware must select the
    HSE clock source (`-DHSE_VALUE=8000000` in `build_flags`). The crystal tolerance is the
    very skew SJW = 4TQ absorbs.

---

## D6 — controlId routing: one format, split at the gateway

**Decision:** Every control on the bus uses the same `ControlPacket` format. The
**`controlId` value alone determines the routing path**, with no extra lookup:

| controlId range | Type | Routed by |
|-----------------|------|-----------|
| `0x0010`–`0x001F` | HID axes | SimGateway → HIDAxis |
| `0x0020`–`0x002F` | HID hat switches | SimGateway → HIDHatSwitch |
| `0x0030`–`0x00AF` | HID buttons | SimGateway → HIDButton |
| `0x00B0`–`0x00FF` | Reserved HID expansion | — |
| `0x8000`–`0x86FF` | DCS-BIOS compact command IDs | PanelBridge → `sendDcsBiosMessage()` |
| `0xFFFF` | Reserved / invalid sentinel | dropped |

Anything `< 0x8000` is HID and is handled entirely at the **SimGateway** layer — PanelBridge
never sees it. Anything in `0x8000`–`0x86FF` is a DCS-BIOS input routed by **PanelBridge**.
Reserved values such as `0xFFFF` are invalid for routing and must never reach the generated
input map.

**Why this design:** The A-4E-C mod has no axis exports — stick and rudder are HID-only — so
the two ranges never overlap, and a single numeric range check is enough to route any packet.
That keeps **PanelGroup nodes dumb**: a node just fires a `controlId` and doesn't care whether
it ends up as a DCS-BIOS command or a joystick axis. **All routing logic lives in one place.**
Add a control type and you change the gateway, not every node.

The practical guidance that falls out of this — when to use DCS-BIOS and when to use HID — is
on the [DCS-BIOS vs HID](dcsbios-vs-hid.md) page.

---

## D7 — STM32F103 for the CAN nodes

**Decision:** The CAN nodes — PanelBridge and every PanelGroup node — use the **STM32F103**
family (LQFP48, 20 KB RAM). The **STM32F103C8** (64 KB flash) is the default; the
**STM32F103CB** (128 KB flash) is used only where flash demands it.
**PanelBridge** is the STM32F103CB case — it runs the DCS-BIOS library plus the full generated
input map (~300 entries). PanelGroup nodes carry only the `DCSIN_*` constants they use,
so they fit the STM32F103C8.

**Reasoning:**

- **Native bxCAN peripheral** — CAN is the backbone of the whole system; a built-in
  controller means no external CAN MCU or SPI-CAN bridge.
- **3.3 V logic** — pairs directly with the SN65HVD230 transceiver, no level shifting.
- **Mature toolchain** — well supported under PlatformIO with the STM32duino Arduino-compatible
  framework, which the firmware is built on.
- **Right-sized and inspectable** — an LQFP48 package the project can hand-assemble and
  visually inspect after reflow.

**The one constraint to know:** USB and CAN share PA11/PA12 and **cannot run at the same
time**. That's exactly why USB lives on the separate RP2040 SimGateway
([D1](#d1-simgateway-exists-because-stm32-usb-cant-take-the-load)) — it lets the STM32
dedicate those pins to CAN. Each board also needs an external 8 MHz crystal for CAN timing
([D5](#d5-sjw-4tq-empirically-determined)).

---

## Related reading

- [CAN Bus Protocol](can-bus.md) — the full frame ID table, `ControlPacket` wire format,
  batching contract, and startup handshake.
- [DCS-BIOS vs HID](dcsbios-vs-hid.md) — the panel-author decision rule and worked examples.
- [Firmware Tiers](firmware-tiers.md) — what each tier does and the contracts between them.
