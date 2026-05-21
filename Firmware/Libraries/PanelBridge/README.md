# PanelBridge

CAN master / UART bridge domain layer for OpenSkyhawk.

`PanelBridge` turns an STM32F103CBT6 into a transparent bridge between the RP2040  
SimGateway (over UART) and the CAN sub-nodes (PanelGroup boards). It does two things:

- **UART → CAN**: `ControlPacket` frames received from the RP2040 are broadcast on  
  the CAN bus as `CTRL_BCAST` frames (or as `TEST_SEQ` frames when `controlId == CTRL_TEST_SEQ`).

- **CAN → UART**: Heartbeat and echo frames received from sub-nodes are forwarded to  
  the RP2040 as 8-byte DIAG frames (`DIAG_MAGIC` framing, defined in `CANProtocol.h`).

A heartbeat watchdog fires `onNodeDead()` if no heartbeat is seen for 3 seconds, and  
`onNodeAlive()` when one returns.

## Dependencies

- [STM32Board](../STM32Board/README.md)
- [CANProtocol](../CANProtocol/) — `ControlPacket`, CAN IDs, `DIAG_*` constants

## UART wiring

Connect UART2 (PA2/PA3) on the master STM32 to GP0/GP1 (Serial1) on the RP2040:

| Signal | Master STM32 | RP2040 |
|---|---|---|
| TX | PA2 (UART2 TX) | GP1 (Serial1 RX) |
| RX | PA3 (UART2 RX) | GP0 (Serial1 TX) |
| GND | GND | GND |

Both sides are 3.3 V — no level shifter required. Baud rate: **250000**.

> **Note:** Use `Serial2` (UART2 on PA2/PA3) on the master STM32, not `Serial` remapped  
> to UART1. Remapping `Serial` to PA9/PA10 causes "multiple definition of Serial2"  
> compile errors with STM32duino.

## Usage

```cpp
#include <PanelBridge.h>

void onAlive(uint8_t nodeId) { /* node came online */ }
void onDead(uint8_t nodeId)  { /* node timed out  */ }

void setup() {
    PanelBridge::onNodeAlive(onAlive);   // optional
    PanelBridge::onNodeDead(onDead);     // optional
    STM32Board::setDebug(true);          // optional
    PanelBridge::setup(Serial2);         // UART2 PA2/PA3 @ 250000
}

void loop() {
    PanelBridge::loop();
}
```

## DIAG frame format

Frames forwarded to the RP2040 are 8 bytes with the first byte always `DIAG_MAGIC` (0xAA):

| Byte | DIAG_RTT | DIAG_HB |
|---|---|---|
| 0 | `0xAA` (DIAG_MAGIC) | `0xAA` |
| 1 | `0x01` (DIAG_RTT) | `0x02` (DIAG_HB) |
| 2–3 | seq (uint16_t LE) | node_id, flags |
| 4–7 | original send timestamp (uint32_t LE) | rxCount (uint16_t LE), padding |

## API

See [`PanelBridge.h`](PanelBridge.h) for full Doxygen documentation.

| Function | Description |
|---|---|
| `PanelBridge::setup(uartPort)` | Init hardware, start UART + CAN |
| `PanelBridge::loop()` | Drain UART + CAN, watchdog check |
| `PanelBridge::onNodeAlive(cb)` | Callback when a sub-node heartbeat appears |
| `PanelBridge::onNodeDead(cb)` | Callback after 3 s with no heartbeat |
