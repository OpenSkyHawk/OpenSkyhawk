# PanelBridge

STM32 CAN master and DCS-BIOS processing node for OpenSkyhawk.

`PanelBridge` turns an STM32F103CBT6 into the CAN cluster master. It runs the DCS-BIOS
library on `Serial` (UART2, PA2/PA3 — wired to SimGateway), batches DCS cockpit-output
updates to `CTRL_BCAST` CAN frames, and routes panel input events (CAN EVTs) to either
DCS-BIOS ASCII commands or 6-byte HID frames back on the same UART.

## Architecture

```
SimGateway (RP2040)
    ↕ UART2 (PA2/PA3) @ 250000 baud
PanelBridge (STM32F103CBT6)   ← this library
    ↕ CAN @ 500 kbps
    ├── PanelGroup #1
    └── PanelGroup #N
```

**DCS-BIOS exports (sim → cockpit):** `BridgeExportListener` listens to the DCS-BIOS
stream on addresses `0x8000–0x86FF` and batches each value as a `ControlPacket` to
`CANProtocol::sendBatched(CAN_ID_CTRL_BCAST)`. PanelGroup nodes subscribe to these frames
to drive LEDs, gauges, and other outputs.

**Panel inputs (cockpit → sim):** CAN EVT frames from PanelGroup nodes carry pairs of
`ControlPacket` values. PanelBridge dispatches each slot:
- `controlId 0x8000–0x86FF` → binary search in `A4EC_INPUT_MAP` → `sendDcsBiosMessage()`
  → DCS-BIOS ASCII command on UART (forwarded by SimGateway to PC)
- `controlId < 0x8000` → 6-byte HID frame (`0xAA 0x55 controlId[2LE] value[2LE]`) on UART
  → SimGateway routes to USB HID axis/button report

## Dependencies

- [STM32Board](../STM32Board/README.md)
- [CANProtocol](../CANProtocol/README.md)
- A4EC — input map and command IDs
- DCS-BIOS — `ExportStreamListener`, `StringBuffer`, `sendDcsBiosMessage()`

## UART wiring

| Signal | STM32 (PanelBridge) | RP2040 (SimGateway) |
|---|---|---|
| TX | PA2 (UART2 TX) | UART RX pin |
| RX | PA3 (UART2 RX) | UART TX pin |
| GND | GND | GND |

Both sides 3.3 V — no level shifter required. Baud rate: **250000**.

## Usage

```cpp
#define DCSBIOS_DEFAULT_SERIAL
#include <DcsBios.h>
#include <PanelBridge.h>
#include <STM32Board.h>

void onAlive(uint8_t nodeId) { /* node came online */ }
void onDead(uint8_t nodeId)  { /* node timed out  */ }

void setup() {
    STM32Board::setDebug(true);          // optional — enables DiagSerial output
    PanelBridge::onNodeAlive(onAlive);   // optional
    PanelBridge::onNodeDead(onDead);     // optional
    PanelBridge::setup();
    DcsBios::setup();
}

void loop() {
    DcsBios::loop();
    PanelBridge::loop();
}
```

`DCSBIOS_DEFAULT_SERIAL` must be defined **before** `#include <DcsBios.h>` in the sketch.
It makes the DCS-BIOS library own `Serial` (UART2). `PanelBridge.h` must be included
**after** `DcsBios.h`.

## Node tracking

PanelBridge tracks up to 63 PanelGroup nodes. A node is marked alive on first heartbeat
or READY frame; dead after 3 s without a heartbeat. On each alive transition, PanelBridge
broadcasts `SYNC_REQ` so the node re-polls and sends its current input state.

## DiagSerial test sequence

Sending `'T'` on DiagSerial (USART1, PA9/PA10, 115200) triggers a `TEST_SEQ` CAN frame
to all nodes. Each node replies with an `ECHO` frame; PanelBridge logs the RTT.

## API

See [`PanelBridge.h`](PanelBridge.h) for full Doxygen documentation.

| Function | Description |
|---|---|
| `PanelBridge::setup()` | Init board services, UART, CAN, cold-boot SYNC_REQ |
| `PanelBridge::loop()` | Drain CAN, check heartbeat timeouts, handle DiagSerial |
| `PanelBridge::onNodeAlive(cb)` | Callback fired when a node transitions to alive |
| `PanelBridge::onNodeDead(cb)` | Callback fired after 3 s with no heartbeat |
