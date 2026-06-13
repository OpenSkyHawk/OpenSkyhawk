# Bring-Up & Testing

First power-on to a verified DCS connection, in order. Don't skip ahead — each step gates the
next. Use the [status LED](../firmware/debugging.md#the-status-led) and DiagSerial as your
instruments throughout.

!!! note "Photos coming"
    The textual procedure is complete; bench photos are still being produced — **(photo TBD)**.

## 1. Power and rails

Before flashing anything, power the board from the bench supply (or bus) and confirm:

- The **3.3 V rail** is at 3.3 V (AMS1117 output).
- No part is getting hot. If anything is, kill power — recheck assembly.

*(photo TBD — multimeter on the 3.3 V test point)*

## 2. Flash and watch the status LED

[Flash](flashing.md) the firmware, then read the bi-color status LED:

| LED | Meaning |
|-----|---------|
| Red blinking | Booting / initialising |
| Green blinking | Normal — CAN bus healthy |
| Red fast blink | TEC > 0 — transmit errors |
| Red solid | Bus-off |
| Amber flicker | Warning / degraded |

Red-solid or red-fast on a single board usually means **no other node / no termination** — CAN
needs 120 Ω at the two end nodes. Confirm bus wiring before chasing firmware.

## 3. DiagSerial

Attach a USB-to-TTL adapter to the 3-pin debug header, open the monitor at **115200**. With
`STM32Board::setDebug(true)`, the board narrates boot, NODE_ID, and CAN status. Confirm the
NODE_ID matches what you claimed.

## 4. Join the bus

Connect PanelBridge and the SimGateway. On a healthy bus the node settles to **green blinking**.
PanelBridge broadcasts `SYNC_REQ`; the node completes its input poll and replies `READY`, then
re-sends its current input state. Gauge steppers home to position 0 at boot (mechanical-stop
homing) before any DCS data arrives — so no random needle motion.

## 5. End-to-end in DCS

Start DCS-BIOS export and load the A-4E-C. Then:

- **Outputs:** trigger the sim state (e.g. a caution light) and confirm the LED/gauge follows.
- **Inputs:** flip a switch and confirm the cockpit state changes in the sim.

The [E2E_DCS_Test](first-panel.md) example is the simplest case — Master Test lights the gear
light through the full loop.

## Troubleshooting

| Symptom | Likely cause |
|---------|--------------|
| Red solid, single board | No bus partner / missing 120 Ω termination |
| ST-Link "tap not found" | Clone JTAG ID — set `CPUTAPID 0x2ba01477` |
| Nothing on DiagSerial | `setDebug(false)`, wrong baud, or TX/RX swapped |
| Output never updates | DCS-BIOS export not running, or wrong `A_4E_C_*` address/mask |
| Input does nothing | Wrong `controlId` (DCS-BIOS vs HID), or pull-up missing |

!!! note "Full diagnostics reference"
    Heartbeats, error counters, and the `SYNC_REQ`/`READY` handshake are detailed under the CAN
    protocol — see [CAN Bus Protocol](../architecture/can-bus.md).
